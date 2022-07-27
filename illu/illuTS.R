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
setwd("//home/jo/Documents/Scolyte/S2/illustration")
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


# création d'une ligne du temps qui suit la fonction harmonique pour l'animation vidéo de RWII
setwd("/home/jo/Documents/Scolyte/planetScope/Scolyte_1/mosaic_period")
source("/home/jo/app/s2/illu/textBox.R")
d1 <- lubridate::ymd( "2018-01-01" ) + lubridate::weeks( 39 - 1 )
d2 <- lubridate::ymd( "2022-01-01" ) + lubridate::weeks( 22 - 1 )
m.dates <- as.Date(c(d1,d2))
dtheorique <- courbeCRSWIR(m.dates)
nbw <- round(difftime(d2,d1,units="weeks"))

library("terra")
require(lubridate)

#for (y in c(2017:2021)){
  # en fait je fonctionne plutôt avec un pas d'une semaine..
  #week
  #for (m in c("01","02","03","04","05","06","07","08","09","10","11","12")){
    #d <- as.Date(paste0(y,"-",m,"-01"))
for (w in c(0:nbw+1)){
    d <- d1 + lubridate::weeks( w )
    dy <- lubridate::ymd( paste0(year(d),"-01-01"))
    dtheoriqueCurrent <- courbeCRSWIR(c(m.dates[1],d))
    nbWa <- round(difftime(d,dy,units="weeks")) 
    cat(paste0("week nb ",w, "  , soit ", year(d), " semaine ", nbWa, " , ",difftime(d,dy,units="weeks"),"\n"))
    tifName <- paste0(getwd(),"/",year(d),"_",sprintf("%02d",nbWa),".tif")
    if (file.exists(tifName)){
      cat(paste0("process date",tifName))
    if (0){
    png(paste0(year(d),"_",sprintf("%02d",nbWa),".tif.fct.png"),width = 1920,height    = 1080,units     = "px",pointsize = 30,bg = "transparent")
    par(mar = c(0,3,0.5,0.5), mgp = c(1.5,0.2,0), tck = 0.02, cex.lab = 1.2, cex.axis = 0.8,bty="n")
    plot(d,0, col="grey20", xlab="",pch=1, ylim=c(0.4,1.7),ylab="", lwd=2, xlim=m.dates,, xaxt = "n", yaxt = "n")
    lines(dtheorique, col="yellow", lwd=10)
    textBox(as.numeric(max(dy,lubridate::ymd( "2019-01-01" ))), 0.8,as.character(year(d)),cex=2,col="forestgreen",fill ="white")
    # ajout d'une ligne en plus gros trait pour montrer ou on en est dans la série temporelle
    lines(dtheoriqueCurrent, col="red", lwd=15)
    dev.off()
    }
    
   if(1){
    #Philippe n'aime pas les couleurs de la vidéo et il a raison.
     #1 ouverture du raster
     r <- rast(tifName)
     #setMinMax(r)
     #stat <- minmax(r)
     #stat[1,1]# min band 1
     #stat[2,1]#max band 1
     stat <- global(r, fun=quantile, na.rm=T, probs=c(0.05,0.95,0.01))
     #cmd <- paste0("otbcli_BandMathX -il ",tifName," -out ",tifName,".rgb.jpg uint8 -exp '(im1b1-",stat[1,1],")*255.0/",stat[2,1]-stat[1,1],";(im1b2-",stat[1,2],")*255.0/",stat[2,2]-stat[1,2],";(im1b3-",stat[1,3],")*255.0/",stat[2,3]-stat[1,3],"' -ram 4000 -progress 0")
     cmd <- paste0("otbcli_BandMathX -il ",tifName," -out ",tifName,".rgb.jpg uint8 -exp '(im1b1-",stat[1,1],")*205.0/",stat[1,2]-stat[1,1],";(im1b2-",stat[2,3],")*255.0/",stat[2,2]-stat[2,3],";(im1b3-",stat[3,1],")*255.0/",stat[3,2]-stat[3,1],"' -ram 4000 -progress 0")
     system(cmd)
   }
    
    if(0){
    esy <- year(d)
    if (nbWa<30){esy <- max(2018,year(d)-1)}
    ES.path <-paste0("/home/jo/Documents/Scolyte/planetScope/Scolyte_1/ES_GE/etatSanitaire_",esy,"GE_tmp_masq_evol_co.tif.jpg")
    ES.out <- paste0(tifName,".ES.jpg")
    command <- paste0("cp ",ES.path, " ", ES.out)
    system(command)
    }
    }
}
