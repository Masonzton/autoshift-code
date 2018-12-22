from tkinter import *
#import tkinter
#tkinter comes preinstalled on windows python,
#not sure for other os
import time
from math import sin,cos,pi
import serial

def x(r,xc=400): #xc is where tac is centered
    '''x position along circle'''
    return 100*cos((330/35*r+90)*pi/180)+xc
def y(r,yc=200): #yc is where tac is centered
    '''y position along circle'''
    return 100*sin((330/35*r+90)*pi/180)+yc
#open serial port
#ser = serial.Serial(port='/dev/cu.DSDTECHHC-06-DevB',baudrate=9600,timeout=0)
ser = serial.Serial(port='/dev/cu.usbmodem14101',baudrate=9600,timeout=0)
print("connected to: " + ser.portstr)

#start tkinter
gui = Tk()
gui.geometry("800x800")
bkgcolor = 'orange'#background color
arwcolor = 'blue'#color of arrow
c = Canvas(gui ,width=800 ,height=800,bg=bkgcolor)
c.pack()
gui.title("RPM sensor")
#define class of tachometer
class tachometer:
    def __init__(self,xc,yc):
        self.xc= xc #x center
        self.yc=yc  #y center
        radius=100
        self.arrow = c.create_line(xc,yc,x(0,xc),y(0,yc),fill=arwcolor)#color of arrow
        self.rpm_text = c.create_text(x(0,xc),y(0,yc)+100,text='RPM: 0',font='100')
        c.create_oval(xc-radius,yc-radius,xc+radius,yc+radius)
        for i in range(0,40,5):#creates numbers along circle 5 multiples
            c.create_text(x(i,xc)+(x(i,xc)-xc)*0.2,y(i,yc)+(y(i,yc)-yc)*0.2,text=str(i),font='20')
        c.create_rectangle(x(0,xc),y(35,yc),x(35,xc),y(0,yc),
        fill=bkgcolor,outline=bkgcolor)  #colors over last part of tac
    def update(self,rpm):
        c.delete(self.arrow)
        c.delete(self.rpm_text)
        self.rpm_text = c.create_text(x(0,self.xc),y(0,self.yc)+100,text='RPM: '+str(rpm),font='100')
        self.arrow = c.create_line(self.xc,self.yc,x(rpm,self.xc),y(rpm,self.yc),arrow=LAST,fill=arwcolor)#color of arrow

tac1 = tachometer(200,300)

while True:
  time.sleep(.1)
  line = ser.readline()
  if line:
      #this code reads the rpm value
      string_line = str(line)
      a=-5 #-1 if using print; -5 if using println for serial port
      if string_line[2:a]!='':
          rpm = int(string_line[2:a])
          rpm = rpm/100 ##### scale by 100 later

      #update rpm value
      #rpm value is meassred in 100's of rpm's from 0 to 35 hundred
      tac1.update(rpm)

  gui.update()

gui.mainloop()
ser.close()
