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

def open_blinds():
    ard.open_blinds()

def close_blinds():
    ard.close_blinds()

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
        top = tk.Frame(self, bg="black", relief="raised")
        top.grid(row=0, column=0, sticky="w")
        title = tk.Label(top, text="JRM - Domotica", font=LARGE_FONT, bg="black", fg="white")
        title.grid(row=0, column=0, sticky="nsew")


        # **** Switching between frames in tkinter(1) ****
        # Source: https://stackoverflow.com/questions/7546050/switch-between-two-frames-in-tkinter
        #Container
        container = tk.Frame(self)
        #container.pack(side="top", fill="both", expand=True)
        container.grid(row=1, column=0)
        #container.grid_rowconfigure(0, minsize=600)
        #container.grid_columnconfigure(0, minsize=800)
        #print(container.grid_location(x,y))
        # **** Switching between frames in tkinter(2) ****
        #Different pages library method
        self.frames = {}
        #Add pages (classes) to tuple in loop
        for F in (StartPage, Settings, ControlUnit, Graph): # Settings, ControlUnit, Graph
            frame = F(container, self)
            self.frames[F] = frame
            frame.grid(row=1, column=0, sticky="nsew")
        #Initialise screen
        #frame = self.frames[StartPage]
        #frame.tkraise()
        self.show_frame(StartPage)
        #Status bar
        #statustext = "ASDASDASDAD"
        #statusbar = tk.Frame(self, bg="blue")
        #statusLabel = tk.Label(text=statustext, bd=1, relief="sunken", anchor="w", fg="white", bg="blue")
        #statusLabel.grid(row=1, column=0, sticky="w")
        print(self.frames)
    # **** Switching between frames in tkinter(3) ****
    #Show frame function
    def show_frame(self, page):
        frame = self.frames[page]
        #frame.grid(row=1, column=0, sticky="nsew")
        #container.grid_remove
        #self.container = frame
        #frame.grid(row=1, column=0, sticky="w")
        #container.grid_rowconfigure(0, weight=1, minsize=600)
        #container.grid_columnconfigure(0, weight=1, minsize=800)
        frame.tkraise()

class StartPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        #parent.grid_remove()
        tempUnit = tk.Frame(self, relief="sunken", bg="black")
        tempUnit.grid(row=1, column=0)
        #Text Animation test
        label = ttk.Label(tempUnit, text="Not connected")
        label.grid(row=0, column=0)
        label.grid_columnconfigure(0)
        label.grid_rowconfigure(0)


        #Page buttons
        settingsbutton = ttk.Button(tempUnit, text="Go to settings", command=lambda: controller.show_frame(Settings))
        settingsbutton.grid(row=1, column=0)
        controlunitbutton = ttk.Button(tempUnit, text="Go to control unit", command=lambda: controller.show_frame(ControlUnit))
        graphbutton = ttk.Button(tempUnit, text="Graphs", command=lambda: controller.show_frame(Graph))
        controlunitbutton.grid(row=2, column=0)
        graphbutton.grid(row=3, column=0)

class Settings(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)

        def save_max_temp():
            maxtemp = maxtemp_entry.get()
            ard.set_temperature_value_to_close(int(maxtemp))

        def save_min_temp():
            mintemp = mintemp_entry.get()
            ard.set_temperature_value_to_open(int(mintemp))

        def save_max_light():
            maxlight = maxlight_entry.get()
            ard.set_light_value_to_close(int(maxlight))

        def save_min_light():
            minlight = minlight_entry.get()
            ard.set_light_value_to_open(int(minlight))

        def save_max_distance():
            maxdistance = maxdistance_entry.get()
            ard.set_closed_distance(int(maxdistance))

        def save_min_distance():
            mindistance = mindistance_entry.get()
            ard.set_open_distance(int(mindistance))


        label = ttk.Label(self, text="Settings", font=LARGE_FONT)
        label.grid(row=0, column=0, pady=10,padx=10)

        maxtemp_setting_label = ttk.Label(self, text="(To close)Max temp: ")
        maxtemp_setting_label.grid(row=1, column=0)
        maxtemp_entry = ttk.Entry(self)
        maxtemp_entry.grid(row=1, column=1)

        mintemp_setting_label = ttk.Label(self, text="(To open)Min temp: ")
        mintemp_setting_label.grid(row=2, column=0)
        mintemp_entry = ttk.Entry(self)
        mintemp_entry.grid(row=2, column=1)

        maxlight_setting_label = ttk.Label(self, text="(To close)Max light: ")
        maxlight_setting_label.grid(row=3, column=0)
        maxlight_entry = ttk.Entry(self)
        maxlight_entry.grid(row=3, column=1)

        minlight_setting_label = ttk.Label(self, text="(To open)Min light: ")
        minlight_setting_label.grid(row=4, column=0)
        minlight_entry = ttk.Entry(self)
        minlight_entry.grid(row=4, column=1)

        maxdistance_setting_label = ttk.Label(self, text="(closed)Max distance: ")
        maxdistance_setting_label.grid(row=5, column=0)
        maxdistance_entry = ttk.Entry(self)
        maxdistance_entry.grid(row=5, column=1)

        mindistance_setting_label = ttk.Label(self, text="(open)Min distance: ")
        mindistance_setting_label.grid(row=6, column=0)
        mindistance_entry = ttk.Entry(self)
        mindistance_entry.grid(row=6, column=1)


        saveMaxTempbutton = ttk.Button(self, text="Save", command=save_max_temp)
        saveMaxTempbutton.grid(row=1,column=2)

        saveMinTempbutton = ttk.Button(self, text="Save", command=save_min_temp)
        saveMinTempbutton.grid(row=2,column=2)

        saveMaxLightbutton = ttk.Button(self, text="Save", command=save_max_light)
        saveMaxLightbutton.grid(row=3,column=2)

        saveMinLightbutton = ttk.Button(self, text="Save", command=save_min_light)
        saveMinLightbutton.grid(row=4,column=2)

        saveMaxDistancebutton = ttk.Button(self, text="Save", command=save_max_distance)
        saveMaxDistancebutton.grid(row=5,column=2)

        saveMinDistancebutton = ttk.Button(self, text="Save", command=save_min_distance)
        saveMinDistancebutton.grid(row=6,column=2)

        backbutton = ttk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.grid(row=6, column=3)

        openblinds = ttk.Button(self, text="Open blinds", command=open_blinds)
        openblinds.grid(row=7,column=2)

        closeblinds = ttk.Button(self, text="Close blinds", command=close_blinds)
        closeblinds.grid(row=7,column=3)

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
            connected = "Connected: True"
            connected = "Connected: " + str(ard.arduino_connected())

            light = "Lightvolume: " + str(ard.get_light())
            temperature = "Temperature: " +  str(ard.get_temperature())
            if ard.get_blinds_status() is 0:
                status = "closed"
            elif ard.get_blinds_status() is 1:
                status = "open"
            elif ard.get_blinds_status() is 2:
                status = "scrolling"
            blinds = "Blind status: " + status
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


        #canvas = FigureCanvasTkAgg(f, self)
        #canvas.show()
        #toolbar = NavigationToolbar2TkAgg(canvas, self)
        #toolbar.update()
        #canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        #canvas._tkcanvas.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

class topMenu(tk.Menu):
    def __init__(self):
        tk.Menu.__init__(self)


app = GUI()
#anima = animation.FuncAnimation(f, animate, interval=1000)

#Full screen with start menu
#app.state('zoomed')

#Full screen without start menu
# app.wm_attributes('-fullscreen', 1)

app.mainloop()
ard.stop()
#Delete random testdata
open("testdata.txt", 'w').close()
