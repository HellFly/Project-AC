#Imports
import tkinter as tk
import matplotlib.animation as animation
import matplotlib
matplotlib.use("TkAgg")
import random
from classes.arduino import Arduino
from tkinter import ttk
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2TkAgg
from matplotlib.figure import Figure
from matplotlib import style

LARGE_FONT = ("Verdana", 12)
a = Arduino(0,0)
style.use("ggplot")

f = Figure(figsize=(5,5), dpi=100)
a = f.add_subplot(111)

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

class GUI(tk.Tk):

    def __init__(self, *args, **kwargs): #Args, kwargs

        tk.Tk.__init__(self, *args, **kwargs)
        tk.Tk.iconbitmap(self, default="images/arduino_icon.ico")
        tk.Tk.wm_title(self, "They see me rollin")

        #Topframe
        topRow = tk.Frame(self,bg="blue")
        topRow.pack(side="top", fill="x", expand = True)
        topRow.grid_rowconfigure(0, weight=1)
        topRow.grid_columnconfigure(0, weight=1)

        #Container
        container = tk.Frame(self)
        container.pack(side="bottom", fill="both", expand = True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        #Different pages library
        self.frames = {}
        #Add pageclasses to tuple in loop
        for F in (StartPage, Settings, ControlUnit, Graph):
            frame = F(container, self)
            self.frames[F] = frame
            frame.grid(row=0, column=0, sticky="nsew")

        #Initialise screen

        self.show_frame(StartPage)

    #Show frame function
    def show_frame(self, cont):
        frame = self.frames[cont]
        frame.tkraise()

class StartPage(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = ttk.Label(self, text="Central unit control", font=LARGE_FONT)
        label.pack(pady=10,padx=10)

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
        label = ttk.Label(self, text="Unit Name (unit.Name from arduino file)", font=LARGE_FONT)
        label.pack(pady=10,padx=10)
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
        canvas.show()
        toolbar = NavigationToolbar2TkAgg(canvas, self)
        toolbar.update()
        canvas.get_tk_widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True)
        canvas._tkcanvas.pack(side=tk.BOTTOM, fill=tk.BOTH, expand=True)

app = GUI()
anima = animation.FuncAnimation(f, animate, interval=1000)

app.mainloop()
#Delete random testdata
open("testdata.txt", 'w').close()
