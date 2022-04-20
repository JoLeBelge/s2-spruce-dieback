# illustration de l'analyse de la série temporelle d'image sentinel 2 pour détection des scolyte - 2021 05 07

# les données input sont celles délivrée par l'appli s2_timeSerie avec le mode XYTest.
#-----------------------------------
require(lubridate)
# une fonction qui renvoie le crswir théorique pour une date
crswirTheorique <- function (date){
  mT <- 365.25
  cst <- 2*pi/mT
 # a1 <- 0.551512
#  b1 <-  0.062849      
 # b2 <- -0.120683
#  b3 <-  0.005509
 # b4 <- -0.044525 
  #nouvelles valeurs;
  a1 <- 0.58880089  
  b1 <- 0.05537490 
  b2 <- -0.10809739 
  b3 <- -0.01737327 
  b4 <- -0.02677137
  x <- yday(date)
  return(a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x))
}

# une fonction qui renvoie des dates jour par jour et le crswir théorique pour ces date
courbeCRSWIR <- function (dates){
  date <-as.Date(min(dates):max(dates), origin="1970-01-01")
  #CRWSIR <- crswirTheorique(vAllDates) 
  CRWSIR <- crswirTheorique(date) 
  #plot(vAllDates,vAllCRWSIR)
  return( data.frame(date,CRWSIR))
}

# input ; détails pour un point
setwd("/home/lisein/Documents/Scolyte/S2/illustration")
# mes couleurs attribuées à chaque état pour une date
mycol <- c("forestgreen", "red" , "black", "purple", "yellow", "royalblue1")
# le type de point (étoile, rond) attribué en fonction de l'état pour une date
mypch <- c(1, 8 , 4, 4, 8, 2)

d <- read.table("scol_1.txt", header=T)
d$date <-  as.Date(paste0(substr(d$date, 7, 10),"-",substr(d$date, 4, 5),"-",substr(d$date, 1, 2)))

# deuxième jeu de donnée
./s2_timeSerie --xmlIn ./s2_scoTestT31UFR.xml --XYtest 659972 5516400 --nbJourStress 150
Test pour une position : 659972,5.5164e+06 avec seuil ratio CRSWIR/CRSWIRtheorique(date) =1.7 et nombre de jours de stress après lesquel on interdit un retour à la normal de 150, tuile T31UFR
d <- read.table("scol_2.txt", header=T, sep=";")
d$date <-  as.Date(d$date)

etatcol <- rep("forestgreen", nrow(d))
etatpch <- rep(1, nrow(d))
for (i in 1:nrow(d)){
  etatcol[i] <- mycol[d$etatFinal[i]]
  etatpch[i] <- mypch[d$etatFinal[i]]
  #etatcol[i] <- mycol[d$etat[i]]
}

pdf(paste0("scol_1.pdf"),width = 9, height = 7)
plot(d$date,d$CRSWIR, col=etatcol, xlab="Date", ylab="complex ratio SWIR", main="suivi de l'état sanitaire des pessières par télédétection",pch=etatpch, ylim=c(0.3,1.7), lwd=1.6)
dtheorique <- courbeCRSWIR(d$date)
lines(dtheorique, col="green")
# le seuil à partir duquel on consière un stress
lines(dtheorique$date,dtheorique$CRWSIR*1.55, col="green", lty= "dashed", lwd=1.5)
legend(dtheorique$date[10], 1.7, legend=c("évolution saisonière du CRSWIR", "seuil pour la détection de stress"),
       col=c("green", "green"), lty=1:2, cex=0.8)
legend(dtheorique$date[10], 1.5, legend=c("sain", "scolyté","coupé sans stress","coupé après stress", "stress temporaire", "stress temporaire hivernal"),
       col=mycol, pch=mypch, cex=0.8)
dev.off()


# deuxieme version pour papier
keep <- d$date > "2017-01-01" & d$date< "2018-10-15"
m.dates <- as.Date(c("2016-01-01","2019-01-01"))

pdf(paste0("scol_2.pdf"),width=9,height=5,colormodel='cmyk')
par(mar = c(3,3,0.5,0.5), mgp = c(1.5,0.2,0), tck = 0.02, cex.lab = 1.2, cex.axis = 0.8,bty="n")
plot(d$date[keep],d$CRSWIR[keep], col="grey20", xlab="time", ylab="Vegetation Indice (SWIR continuum removal)",pch=1, ylim=c(0.4,1.3), lwd=2, xlim=m.dates)
dtheorique <- courbeCRSWIR(m.dates)
# un point est sous la courbe
points(d$date[keep],d$CRSWIR[keep], col="grey20", lwd=2,pch=1)

lines(dtheorique, col="forestgreen", lwd=4)
# le seuil à partir duquel on consière un stress
lines(dtheorique$date[dtheorique$date>"2017-01-01"],dtheorique$CRWSIR[dtheorique$date>"2017-01-01"]*1.7, col="forestgreen", lty= "dashed", lwd=2)
dev.off()



# calibration de la fct harmonique sur les points pessière saine en Ardenne pour voir la différence avec la courbe de l'INRAE
setwd("/home/lisein/Documents/Scolyte/S2/build-s2_ts/ptSain")
l.data <- list.files(path = ".")

dsain <- employ.data <- data.frame(employee=as.Date(), salary, startdate)
df <- data.frame(date=as.Date(character()),
                 DIY=integer(), 
                 crswir=double(), 
                 stringsAsFactors=FALSE) 

for (f in l.data){
  cat(paste0(f,"\n"))
  d <-read.csv(f, sep=";")
  
  dfadd <- data.frame(date=as.Date(d$date),
                      DIY= yday(d$date), 
                      crswir=d$CRSWIR, 
                      stringsAsFactors=FALSE) 
  df  <- rbind(df, dfadd)
}

plot(df$DIY,df$crswir, main="variation saisonière du CRSWIR en Belgique pour pessière saine")
mT <- 365.25
cst <- 2*pi/mT
a1 <- 0.551512
b1 <-  0.062849      
b2 <- -0.120683
b3 <-  0.005509
b4 <- -0.044525 
x <- 1:365
y <- (a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x))
lines(x,y, col="green", lwd=3)

library(nls2)
formule <- crswir  ~  aa1 + bb1*sin(cst*DIY)+ bb2*cos(cst*DIY)+ bb3*sin(cst*2*DIY)+ bb4*cos(cst*2*DIY)
fit <- nls2(formule, trace=T, data=df,start = list(aa1 = a1, bb1 = b1, bb2=b2,bb3=b3,bb4=b4))



y <- (a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x))
lines(x,y, col="forestgreen", lwd=4)



#----
#suivi de pixel en hetraie dépérissante

setwd("/home/gef/Documents/he-reponse")

for (id in c(1:20)){
d <- read.table(paste0("toto_",id,".txt"), header=T,sep=";") # il est mort le numéro 13
d$date <-  as.Date(d$date)
d$ndvi <-(d$B8A-d$B4)/(d$B8A+d$B4)
pdf(paste0("he-ndvi",id,".pdf"),width = 9, height = 7)
plot(d$date,d$CRSWIR, xlab="Date", ylab="complex ratio SWIR", main="suivi de l'état sanitaire des hêtres par télédétection", ylim=c(0.3,1.7), lwd=1.6)

#plot(d$date,d$ndvi, xlab="Date", ylab="complex ratio SWIR", main="suivi de l'état sanitaire des hêtres par télédétection", ylim=c(0.0,1.0), lwd=1.6)



dev.off()
}

##################### illustration synthèse radiométrique trimestrielle ###########

setwd("/home/lisein/Documents/Scolyte/S2/illustration")

d <- read.table("TestSynthesePheno.txt", header=T,sep=";")
# un dataframe pour les moyennes, un autre pour les observations brutes
dm <-  d[d$type=="m",]
dm$date <- as.Date(paste("2000",c("02","05","08","11"),"15", sep="-"))
do <-  d[d$type=="o",]
do$date <- as.Date(do$date)
do$date2 <- as.Date(paste("2000",format(do$date, "%m"),format(do$date, "%d"),sep="-"))

plot(do$date2,do$b8A)
points(dm$date,dm$b8A, col="blue")
