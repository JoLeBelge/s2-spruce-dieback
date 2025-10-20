#ifndef MODEL_H
#define MODEL_H
#include <torch/torch.h>
#include <torch/nn/modules/conv.h>
#include <torch/nn/modules/linear.h>
#include <torch/nn/modules/batchnorm.h>
#include <torch/nn/modules/dropout.h>
#include <torch/nn/modules/activation.h>
#include <torch/nn/modules/container/sequential.h>
//#include <torch/nn/modules/container/module.h>
#include <torch/serialize.h>
#include <string>
#include <iostream>
#include <filesystem>

// Pour la compatibilité avec les systèmes de fichiers
namespace fs = std::filesystem;


struct Conv1D_BatchNorm_Relu_DropoutImpl : torch::nn::Module {
    torch::nn::Sequential block{nullptr};
    Conv1D_BatchNorm_Relu_DropoutImpl(int64_t input_dim, int64_t hidden_dims, int64_t kernel_size=5, double drop_probability=0.5)
        : block(
            torch::nn::Conv1d(torch::nn::Conv1dOptions(input_dim, hidden_dims, kernel_size).padding(kernel_size / 2)),
            torch::nn::BatchNorm1d(hidden_dims),
            torch::nn::ReLU(),
            torch::nn::Dropout(drop_probability)
        ) {
        register_module("block", block);
    }
    torch::Tensor forward(torch::Tensor x) {
        return block->forward(x);
    }
};
TORCH_MODULE(Conv1D_BatchNorm_Relu_Dropout);

struct FC_BatchNorm_Relu_DropoutImpl : torch::nn::Module {
    torch::nn::Sequential block{nullptr};
    FC_BatchNorm_Relu_DropoutImpl(int64_t input_dim, int64_t hidden_dims, double drop_probability=0.5)
        : block(
            torch::nn::Linear(input_dim, hidden_dims),
            torch::nn::BatchNorm1d(hidden_dims),
            torch::nn::ReLU(),
            torch::nn::Dropout(drop_probability)
        ) {
        register_module("block", block);
    }
    torch::Tensor forward(torch::Tensor x) {
        return block->forward(x);
    }
};
TORCH_MODULE(FC_BatchNorm_Relu_Dropout);

struct FlattenImpl : torch::nn::Module {
    torch::Tensor forward(torch::Tensor x) {
        return x.view({x.size(0), -1});
    }
};
TORCH_MODULE(Flatten);

struct TempCNNImpl : torch::nn::Module {
    std::string modelname;
    int64_t hidden_dims;
    Conv1D_BatchNorm_Relu_Dropout conv_bn_relu1{nullptr};
    Conv1D_BatchNorm_Relu_Dropout conv_bn_relu2{nullptr};
    Conv1D_BatchNorm_Relu_Dropout conv_bn_relu3{nullptr};
    Flatten flatten{nullptr};
    FC_BatchNorm_Relu_Dropout dense{nullptr};
    torch::nn::Sequential logsoftmax{nullptr};

    TempCNNImpl(int64_t input_dim=13, int64_t num_classes=9, int64_t sequencelength=45, int64_t kernel_size=7, int64_t hidden_dims=128, double dropout=0.18203942949809093)
        : hidden_dims(hidden_dims),
          conv_bn_relu1(input_dim, hidden_dims, kernel_size, dropout),
          conv_bn_relu2(hidden_dims, hidden_dims, kernel_size, dropout),
          conv_bn_relu3(hidden_dims, hidden_dims, kernel_size, dropout),
          flatten(Flatten()),
          dense(hidden_dims * sequencelength, 4 * hidden_dims, dropout),
          logsoftmax(torch::nn::Linear(4 * hidden_dims, num_classes), torch::nn::LogSoftmax(-1)) {
        modelname = "TempCNN_input-dim=" + std::to_string(input_dim) +
                    "_num-classes=" + std::to_string(num_classes) +
                    "_sequencelenght=" + std::to_string(sequencelength) +
                    "_kernelsize=" + std::to_string(kernel_size) +
                    "_hidden-dims=" + std::to_string(hidden_dims) +
                    "_dropout=" + std::to_string(dropout);
        register_module("conv_bn_relu1", conv_bn_relu1);
        register_module("conv_bn_relu2", conv_bn_relu2);
        register_module("conv_bn_relu3", conv_bn_relu3);
        register_module("flatten", flatten);
        register_module("dense", dense);
        register_module("logsoftmax", logsoftmax);
    }

    torch::Tensor forward(torch::Tensor x) {
        x = x.transpose(1, 2);
        x = conv_bn_relu1->forward(x);
        x = conv_bn_relu2->forward(x);
        x = conv_bn_relu3->forward(x);
        x = flatten->forward(x);
        x = dense->forward(x);
        return logsoftmax->forward(x);
    }

    /*void save(const std::string& path) {
        std::cout << "\nsaving model to " << path << std::endl;
        fs::create_directories(fs::path(path).parent_path());
        torch::save(this, path);
    }

    void load(const std::string& path) {
        std::cout << "loading model from " << path << std::endl;
        torch::load(this, path);
    }*/
};
TORCH_MODULE(TempCNN);




#endif // MODEL_H
