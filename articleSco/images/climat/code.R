
#package
require(ggplot2)
require(ggrepel)

#graphe
pl <-ggplot(d, aes(P,Tmean, shape=as.factor(y)))+
  geom_point(aes(colour= region), size=2)+
  scale_color_manual(values=c("red","blue"))+
  geom_text_repel(aes(label = loca_str),
                  size = 3.5)+
  #ggtitle("Moyenne sur la période de végétation")+
  xlab("Average precipitation during the growing season (mm)") + ylab("Average temperature during the growing season (°C)")+
  theme(
    # Masquer les bords des panneaux et supprimer les lignes de grilles
    panel.border = element_blank(),
    panel.grid.major = element_blank(),
    panel.grid.minor = element_blank(),
    # Modifier le trait des axes
    axis.line = element_line(colour = "black")
  )+
  theme_classic()
###################################################################"

setwd("/home/jo/app/s2/articleSco/images/climat")
d <- read.table("growing_seasing_r_t_30.csv", sep=";", header=T)
d <- d[order(d$num),]

pStyle <- rep(16,nrow(d))
pStyle[d$region=="Wallonie"]<-18

pdf("climat.pdf",width=5,height=6,colormodel='cmyk')
plot(d$P,d$Tmean, xlim=c(270,580),ylim=c(14,18), pch=pStyle,
     bty="n",xlab="",ylab="" )
mtext("Average precipitation during the growing season (mm)", side=1, line=2.9, cex=1)
mtext("Average temperature during the growing season (°C)", side=2, line=2.4, cex=1)

text(d$P,d$Tmean+0.12,d$num, cex=0.7)
dev.off()
