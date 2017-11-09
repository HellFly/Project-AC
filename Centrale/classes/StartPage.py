import main.py
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
