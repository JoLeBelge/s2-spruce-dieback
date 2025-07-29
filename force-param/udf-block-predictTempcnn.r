# global header, e.g., for loading libraries
library("sits")
library("LDATS")
library("torch")
torch::torch_set_num_threads(6) # fonctionne pas? je sais pas mais un seul coeur à 100%
mod_tempcnn <-readRDS("/home/jo/Documents/S2/dc_tool_out/sits/tempcnn_csw_ndv.rds")
#torch::torch_set_num_interop_threads(6)
#sfExport('mod_tempcnn')

# dates:     vector with dates     of input data (class: Date)
# sensors:   vector with sensors   of input data (class: character)
# bandnames: vector with bandnames of input data (class: character)
force_rstats_init <- function(dates, sensors, bandnames){
    return(c("prob_sain", "prob_depe"))
}

# nproc:     number of CPUs the UDF may use. Always 1 for pixel functions (class: Integer)
force_rstats_block <- function(inarray, dates, sensors, bandnames, nproc){
  
  #saveRDS(inarray,"/home/jo/Documents/S2/dc_tool_out/inarrayC.rds")
  # return array 3D
  #number of output bands (as initialized)
  #number of rows
  #number of columns
  #out <- array(numeric(),c(2,dim(inarray)[3],dim(inarray)[4])) 
  
  torch::torch_set_num_threads(nproc)

  reshaped <- array( aperm(inarray[,2:1,,], c(3,4,1,2)) , dim =c( 250*500, 2*192) )/10000
  # trouver un pixel qui n'as pas des valeurs de NA. u 249, v 158
  # px1 <- 158*500+249
  # test <- reshaped[px1,]
  o <- sits::impute_linear(reshaped)
  predBlock <- mod_tempcnn(o)
  out <- 100*LDATS::softmax(predBlock)
  return (  array( aperm(out, c(2,1)) , dim =c(2, 250,500) ))
}
