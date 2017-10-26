#Imports
from classes.arduino import Arduino
import tkinter as tk

LARGE_FONT = ("Verdana", 12)

class GUI(tk.Tk):
    def __init__(self, *args, **kwargs): #Args, kwargs
        tk.Tk.__init__(self, *args, **kwargs)
        container = tk.Frame(self)
        topRow = 
        container.pack(side="bottom", fill="both", expand = True)
        container.grid_rowconfigure(0, weight=1)
        container.grid_columnconfigure(0, weight=1)

        #Different pages library
        self.frames = {}
        #Add pageclasses to tuple in loop
        for F in (StartPage, Settings, ControlUnit):
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
        label = tk.Label(self, text="Central unit control", font=LARGE_FONT)
        label.pack(pady=10,padx=10)

        #Page buttons
        settingsbutton = tk.Button(self, text="Go to settings", command=lambda: controller.show_frame(Settings))
        settingsbutton.pack()
        controlunitbutton = tk.Button(self, text="Go to control unit", command=lambda: controller.show_frame(ControlUnit))
        controlunitbutton.pack()

class Settings(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = tk.Label(self, text="Settings", font=LARGE_FONT)
        label.pack(pady=10,padx=10)
        backbutton = tk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.pack()

class ControlUnit(tk.Frame):
    def __init__(self, parent, controller):
        tk.Frame.__init__(self, parent)
        label = tk.Label(self, text="Unit Name (unit.Name from arduino file)", font=LARGE_FONT)
        label.pack(pady=10,padx=10)
        backbutton = tk.Button(self, text="Go back", command=lambda: controller.show_frame(StartPage))
        backbutton.pack()

app = GUI()
app.mainloop()
