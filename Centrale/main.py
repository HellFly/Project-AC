#Imports
import tkinter as tk
import matplotlib.animation as animation
import matplotlib
matplotlib.use("TkAgg")
import random
import datetime

from classes.arduino import Arduino
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg
from matplotlib.figure import Figure
from matplotlib import style
from PIL import Image, ImageTk
from time import sleep

#Styles
LARGE_FONT = ("Verdana", 12)
style.use("ggplot")
f = Figure(figsize=(5,5), dpi=100)
a = f.add_subplot(111)

#Initiate arduino
ard = Arduino()

#Globals
#global temperature
#temperature = ard.get_temperature()
#print(temperature)

def animate(i):
    #Testdata
    x = i
    y = random.randint(0,20)
    with open("testdata.txt", "a") as testdata:
        testdata.write(str(x)+","+str(y)+"\n")

    pullData = open("testdata.txt", "r").read()
    dataList = pullData.split('\n')
    xList = []
    yList = []
    for eachLine in dataList:
        if len(eachLine) > 1:
            x,y = eachLine.split(',')
            xList.append(int(x))
            yList.append(float(y))
    a.clear()
    a.plot(xList, yList)

def doNothing():
    print("Nothing")


class GUI(tk.Tk):

    def __init__(self, *args, **kwargs): #Args, kwargs

        tk.Tk.__init__(self, *args, **kwargs)
        tk.Tk.iconbitmap(self, default="images/hqdefault.ico")
        tk.Tk.wm_title(self, "They see me rollin")

        #Topmenu
        tMenu = tk.Menu()
        self.config(menu=tMenu)
        subMenu = tk.Menu(tMenu, tearoff=0)
        tMenu.add_cascade(label="File", menu=subMenu)
        subMenu.add_command(label="Add arduino", command=doNothing)
        subMenu.add_command(label="Save", command=doNothing)
        subMenu.add_separator()
        #Exit program
        subMenu.add_command(label="Exit", command=self.destroy)

        #Title bar
        #top = tk.Frame(self, bg="black")
        #top.grid(row=0, column=0, sticky="w")
        #Title
        title = tk.Label(text="Central unit control", font=LARGE_FONT, bg="black", fg="white")
        title.grid(row=0, column=0, sticky="w")


        # **** Switching between frames in tkinter(1) ****
        # Source: https://stackoverflow.com/questions/7546050/switch-between-two-frames-in-tkinter
        #Container
        container = tk.Frame(self)
        container.grid()
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        # **** Switching between frames in tkinter(2) ****
        #Different pages library method
        self.frames = {}
        #Add pages (classes) to tuple in loop
        for F in (StartPage, Settings, ControlUnit, Graph):
            frame = F(container, self)
            self.frames[F] = frame
            frame.grid(row=0, column=0, sticky="nsew")
        #Initialise screen
        self.show_frame(StartPage)


        #Status bar
        #statustext = "ASDASDASDAD"
        #statusbar = tk.Frame(self, bg="blue")
        #statusLabel = tk.Label(text=statustext, bd=1, relief="sunken", anchor="w", fg="white", bg="blue")
        #statusLabel.grid(row=1, column=0, sticky="w")

    # **** Switching between frames in tkinter(3) ****
    #Show frame function
    def show_frame(self, cont):
        frame = self.frames[cont]
        frame.tkraise()

class StartPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        #global temperature
        #label = ttk.Label(self, text=temperature, font=LARGE_FONT)
        #label.pack(pady=10,padx=10)

        #Text Animation test
        label = ttk.Label(self, text="Not connected")
        label.pack()
        def check():
            label.config(text="Checking")

        checkBtn = ttk.Button(self, text="Check arduino", command=check)
        checkBtn.pack()



        #Page buttons
        settingsbutton = ttk.Button(self, text="Go to settings", command=lambda: controller.show_frame(Settings))
        settingsbutton.pack()
        controlunitbutton = ttk.Button(self, text="Go to control unit", command=lambda: controller.show_frame(ControlUnit))
        graphbutton = ttk.Button(self, text="Graphs", command=lambda: controller.show_frame(Graph))
        controlunitbutton.pack()
        graphbutton.pack()

class Settings(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = ttk.Label(self, text="Settings", font=LARGE_FONT)
        label.pack(pady=10,padx=10)
        backbutton = ttk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.pack()

class ControlUnit(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = ttk.Label(self, text="Arduino", font=LARGE_FONT)
        label.pack(pady=10,padx=10)

        connected_label = ttk.Label(self, text="Connected: ")
        light_label = ttk.Label(self, text="Lightvolume: ")
        temperature_label = ttk.Label(self, text="Temperature: ")
        blinds_label = ttk.Label(self, text="Blind status: ")
        connected_label.pack()
        light_label.pack()
        temperature_label.pack()
        blinds_label.pack()
        def clock():
            connected = "Connected: " + str(ard.arduino_connected())
            light = "Lightvolume: " + str(ard.get_light())
            temperature = "Temperature: " +  str(ard.get_temperature())
            blinds = "Blind status: " +  str(ard.get_blinds_status())
            connected_label.config(text=connected)
            light_label.config(text=light)
            temperature_label.config(text=temperature)
            blinds_label.config(text=blinds)
            self.after(1000, clock)
        clock()

        #Back button
        backbutton = ttk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.pack()

class Graph(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = ttk.Label(self, text="Graph", font=LARGE_FONT)
        label.pack(pady=10,padx=10)
        backbutton = ttk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.pack()


        canvas = FigureCanvasTkAgg(f, self)
        #canvas.show()
        toolbar = NavigationToolbar2TkAgg(canvas, self)
        toolbar.update()
        canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        canvas._tkcanvas.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

class topMenu(tk.Menu):
    def __init__(self):
        tk.Menu.__init__(self)


app = GUI()
anima = animation.FuncAnimation(f, animate, interval=1000)

#Full screen with start menu
#app.state('zoomed')

#Full screen without start menu
# app.wm_attributes('-fullscreen', 1)

app.mainloop()
ard.stop()
#Delete random testdata
open("testdata.txt", 'w').close()
