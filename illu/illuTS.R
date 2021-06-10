# illustration de l'analyse de la série temporelle d'image sentinel 2 pour détection des scolyte - 2021 05 07

# les données input sont celles délivrée par l'appli s2_timeSerie avec le mode XYTest.
#-----------------------------------
require(lubridate)
# une fonction qui renvoie le crswir théorique pour une date
crswirTheorique <- function (date){
  mT <- 365.25
  cst <- 2*pi/mT
  a1 <- 0.551512
  b1 <-  0.062849      
  b2 <- -0.120683
  b3 <-  0.005509
  b4 <- -0.044525 
  x <- yday(date)
  return(a1 + b1*sin(cst*x)+ b2*cos(cst*x)+ b3*sin(cst*2*x)+ b4*cos(cst*2*x))
}

# une fonction qui renvoie des dates jour par jour et le crswir théorique pour ces date
courbeCRSWIR <- function (dates){
  date <-as.Date(min(dates):max(dates), origin="1970-01-01")
  CRWSIR <- crswirTheorique(vAllDates) 
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
