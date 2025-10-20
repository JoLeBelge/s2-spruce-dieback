import sys

sys.path.append("./models")
sys.path.append("..")

import argparse

import breizhcrops
from breizhcrops.models import LSTM, TempCNN, MSResNet, TransformerModel, InceptionTime, StarRNN, OmniScaleCNN, PETransformerModel
from torch.utils.data import DataLoader
from tqdm import tqdm
from torch.optim import Adam
import torch
import pandas as pd
import os
import sklearn.metrics

from torch.utils.data import Dataset
import numpy as np
import re
from osgeo import gdal,osr,ogr
from datetime import datetime, timedelta

class ts_s2(Dataset):

    def __init__(self,
                 root,
                 sample_file="train.csv",
                 train=True
                ):
        self.root = "/home/jo/Documents/S2/deadTree-obAgr"
        #self.obs= pd.read_csv(root+"/"+sample_file, index_col=0)
        self.obs= pd.read_csv(root+"/"+sample_file, index_col=False)
        # determiner la position uv de chaque observation
        self.obs["v"] = 0
        self.obs["u"] = 0

        source = osr.SpatialReference()
        source.ImportFromEPSG(4326)
        target = osr.SpatialReference()
        target.ImportFromEPSG(31370)
        transform = osr.CoordinateTransformation(source, target)

        for j in range(0,len(self.obs)):
            point = ogr.CreateGeometryFromWkt("POINT ("+str(self.obs["latitude"][j])+" "+ str(self.obs["longitude"][j])+")")
            t = point.Transform(transform)
            self.obs.at[j,"u"] = round((point.GetX()-200000-50)/100)
            self.obs.at[j,"v"] = -round((point.GetY()-100000-50)/100)
        
        self.labels = self.obs["label"]
        # il faut stoquer les valeur des deux indices spectraux
        self.ts_csw = np.zeros((len(self.labels), 192))
        self.ts_ndv = np.zeros((len(self.labels), 192))

        self.readDatacube()

    def readDatacube(self):
        print("start datacube loading")
        first_date_str ="2017-01-01"
        date1 = datetime.strptime(first_date_str, "%Y-%m-%d")
        lp=0
        for i in range(0,191):
            print("time "+str(i))
            if (i==73 or i==164):lp+=1
                
            # A partir de 73 ça bugg à cause de la leap year, Force n'as pas compter le 29 fevrier...
            cur_date = date1 + timedelta(days=(16*i)+lp)
            file1=self.root+"/SEN2L_FORCETSI_T1_NDV_"+cur_date.strftime("%Y-%m-%d")+".tif"
            file2=self.root+"/SEN2L_FORCETSI_T1_CSW_"+cur_date.strftime("%Y-%m-%d")+".tif"
            raster1 = gdal.Open(file1)
            bandval1 = raster1.ReadAsArray()
            raster2 = gdal.Open(file2)
            bandval2 = raster2.ReadAsArray()
            for j in range(0,len(self.obs)):
                self.ts_ndv[j,i] = bandval1[self.obs["u"].loc[j],self.obs["v"].loc[j]]
                self.ts_csw[j,i] = bandval2[self.obs["u"].loc[j],self.obs["v"].loc[j]]
        print("datacube loaded")

#number of observations
    def __len__(self):
        return len(self.labels)

    def __getitem__(self, idx):
     
        #label = self.labels.iloc[idx, 1]
        label = self.labels.iloc[idx]
        y=0 if label=="label1" else 1

        #combined = np.concatenate((self.ts_csw[idx], self.ts_ndv[idx]), axis=0)
        #ts = combined.reshape((2, 192)) * 1e-4  # scale reflectances to 0-1
        #ts = combined.reshape((192, 2)) * 1e-4 

        ts = np.vstack((self.ts_csw[idx], self.ts_ndv[idx])) * 1e-4 

        ts= np.transpose(ts, (1, 0))

       # ts.reshape((192, 2)) * 1e-4 
        #combined.reshape((192, 2)) * 1e-4 
    
        return torch.from_numpy(ts).type(torch.FloatTensor), y, idx

def train(args):
    traindataloader, testdataloader, meta = get_dataloader(args.datapath, args.batchsize, args.workers)

    num_classes = meta["num_classes"]
    ndims = meta["ndims"]
    sequencelength = meta["sequencelength"]

    device = torch.device(args.device)
    model = get_model("tempcnn", ndims, num_classes, sequencelength, device, **args.hyperparameter)
    optimizer = Adam(model.parameters(), lr=args.learning_rate, weight_decay=args.weight_decay)
    model.modelname += f"_learning-rate={args.learning_rate}_weight-decay={args.weight_decay}"
    print(f"Initialized {model.modelname}")

    logdir = os.path.join(args.logdir, model.modelname)
    os.makedirs(logdir, exist_ok=True)
    print(f"Logging results to {logdir}")

    criterion = torch.nn.CrossEntropyLoss(reduction="mean")

    log = list()
    for epoch in range(args.epochs):
        train_loss = train_epoch(model, optimizer, criterion, traindataloader, device)
        test_loss, y_true, y_pred, *_ = test_epoch(model, criterion, testdataloader, device)
        scores = metrics(y_true.cpu(), y_pred.cpu())
        scores_msg = ", ".join([f"{k}={v:.2f}" for (k, v) in scores.items()])
        test_loss = test_loss.cpu().detach().numpy()[0]
        train_loss = train_loss.cpu().detach().numpy()[0]
        print(f"epoch {epoch}: trainloss {train_loss:.2f}, testloss {test_loss:.2f} " + scores_msg)

        scores["epoch"] = epoch
        scores["trainloss"] = train_loss
        scores["testloss"] = test_loss
        log.append(scores)

        log_df = pd.DataFrame(log).set_index("epoch")
        log_df.to_csv(os.path.join(logdir, "trainlog.csv"))

def get_dataloader(datapath, batchsize, workers):
    print(f"Setting up datasets from {os.path.abspath(datapath)}")
    datapath = os.path.abspath(datapath)

# dataset

    traindatasets = ts_s2(datapath, sample_file="train.csv", train=True)
    testdataset = ts_s2(datapath,sample_file="val.csv",train=False)
   
# dataloader

    traindataloader = DataLoader(traindatasets, batch_size=batchsize, shuffle=True, num_workers=workers)
    testdataloader = DataLoader(testdataset, batch_size=batchsize, shuffle=False, num_workers=workers)

    meta = dict(
        ndims=2 ,
        num_classes=2,
        sequencelength=192
    )

    return traindataloader, testdataloader, meta


def get_model(modelname, ndims, num_classes, sequencelength, device, **hyperparameter):
    modelname = modelname.lower() #make case invariant
    if modelname == "omniscalecnn":
        model = OmniScaleCNN(input_dim=ndims, num_classes=num_classes, sequencelength=sequencelength, **hyperparameter).to(device)
    elif modelname == "lstm":
        model = LSTM(input_dim=ndims, num_classes=num_classes, **hyperparameter).to(device)
    elif modelname == "starrnn":
        model = StarRNN(input_dim=ndims,
                        num_classes=num_classes,
                        bidirectional=False,
                        use_batchnorm=False,
                        use_layernorm=True,
                        device=device,
                        **hyperparameter).to(device)
    elif modelname == "inceptiontime":
        model = InceptionTime(input_dim=ndims, num_classes=num_classes, device=device,
                              **hyperparameter).to(device)
    elif modelname == "msresnet":
        model = MSResNet(input_dim=ndims, num_classes=num_classes, **hyperparameter).to(device)
    elif modelname in ["transformerencoder","transformer"]:
        model = TransformerModel(input_dim=ndims, num_classes=num_classes,
                            activation="relu",
                            **hyperparameter).to(device)
    elif modelname in ["petransformer"]:
        model = PETransformerModel(input_dim=ndims, num_classes=num_classes,
                                 activation="relu",
                                 **hyperparameter).to(device)
    elif modelname == "tempcnn":
        model = TempCNN(input_dim=ndims, num_classes=num_classes, sequencelength=sequencelength, **hyperparameter).to(
            device)
    else:
        raise ValueError("invalid model argument. choose from 'LSTM','MSResNet','TransformerEncoder', or 'TempCNN'")

    return model

def metrics(y_true, y_pred):
    accuracy = sklearn.metrics.accuracy_score(y_true, y_pred)
    kappa = sklearn.metrics.cohen_kappa_score(y_true, y_pred)
    f1_micro = sklearn.metrics.f1_score(y_true, y_pred, average="micro")
    f1_macro = sklearn.metrics.f1_score(y_true, y_pred, average="macro")
    f1_weighted = sklearn.metrics.f1_score(y_true, y_pred, average="weighted")
    recall_micro = sklearn.metrics.recall_score(y_true, y_pred, average="micro")
    recall_macro = sklearn.metrics.recall_score(y_true, y_pred, average="macro")
    recall_weighted = sklearn.metrics.recall_score(y_true, y_pred, average="weighted")
    precision_micro = sklearn.metrics.precision_score(y_true, y_pred, average="micro")
    precision_macro = sklearn.metrics.precision_score(y_true, y_pred, average="macro")
    precision_weighted = sklearn.metrics.precision_score(y_true, y_pred, average="weighted")

    return dict(
        accuracy=accuracy,
        kappa=kappa,
        f1_micro=f1_micro,
        f1_macro=f1_macro,
        f1_weighted=f1_weighted,
        recall_micro=recall_micro,
        recall_macro=recall_macro,
        recall_weighted=recall_weighted,
        precision_micro=precision_micro,
        precision_macro=precision_macro,
        precision_weighted=precision_weighted,
    )


def train_epoch(model, optimizer, criterion, dataloader, device):
    model.train()
    losses = list()
    with tqdm(enumerate(dataloader), total=len(dataloader), leave=True) as iterator:
        for idx, batch in iterator:
            optimizer.zero_grad()
            x, y_true, _ = batch
            #x, y_true = batch
            #print("test done")
            loss = criterion(model.forward(x.to(device)), y_true.to(device))
            loss.backward()
            optimizer.step()
            iterator.set_description(f"train loss={loss:.2f}")
            losses.append(loss)
    return torch.stack(losses)


def test_epoch(model, criterion, dataloader, device):
    model.eval()
    with torch.no_grad():
        losses = list()
        y_true_list = list()
        y_pred_list = list()
        y_score_list = list()
        field_ids_list = list()
        with tqdm(enumerate(dataloader), total=len(dataloader), leave=True) as iterator:
            for idx, batch in iterator:
                x, y_true, field_id = batch
                logprobabilities = model.forward(x.to(device))
                loss = criterion(logprobabilities, y_true.to(device))
                iterator.set_description(f"test loss={loss:.2f}")
                losses.append(loss)
                y_true_list.append(y_true)
                y_pred_list.append(logprobabilities.argmax(-1))
                y_score_list.append(logprobabilities.exp())
                field_ids_list.append(field_id)
        return torch.stack(losses), torch.cat(y_true_list), torch.cat(y_pred_list), torch.cat(y_score_list), torch.cat(field_ids_list)


def parse_args():
    parser = argparse.ArgumentParser(description='Train an evaluate time series deep learning models on the'
                                                 'BreizhCrops dataset. This script trains a model on training dataset'
                                                 'partition, evaluates performance on a validation or evaluation partition'
                                                 'and stores progress and model paths in --logdir')
    parser.add_argument(
        '-b', '--batchsize', type=int, default=64, help='batch size (number of time series processed simultaneously)')
    parser.add_argument(
        '-e', '--epochs', type=int, default=150, help='number of training epochs (training on entire dataset)')
    parser.add_argument(
        '-D', '--datapath', type=str, default="../data", help='directory of concatenated datacube')
    parser.add_argument(
        '-w', '--workers', type=int, default=0, help='number of CPU workers to load the next batch')
    parser.add_argument(
        '-H', '--hyperparameter', type=str, default=None, help='model specific hyperparameter as single string, '
                                                               'separated by comma of format param1=value1,param2=value2')
    parser.add_argument(
        '--weight-decay', type=float, default=1e-6, help='optimizer weight_decay (default 1e-6)')
    parser.add_argument(
        '--learning-rate', type=float, default=1e-2, help='optimizer learning rate (default 1e-2)')
    parser.add_argument(
        '-l', '--logdir', type=str, default="/tmp", help='logdir to store progress and models (defaults to /tmp)')
    args = parser.parse_args()

    args.kernel_size = 7

    hyperparameter_dict = dict()
    if args.hyperparameter is not None:
        for hyperparameter_string in args.hyperparameter.split(","):
            param, value = hyperparameter_string.split("=")
            hyperparameter_dict[param] = float(value) if '.' in value else int(value)
    args.hyperparameter = hyperparameter_dict
    args.device = "cuda" if torch.cuda.is_available() else "cpu"

    return args


if __name__ == "__main__":
    args = parse_args()

    train(args)


