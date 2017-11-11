import sys
import PyQt5 as PyQt
#from PyQt5.QtWidgets import QApplication, QDialog
from designs.index import index

app = PyQt.QApplication(sys.argv)
window = PyQt.QDialog()
ui = index()
ui.setupUi(window)

window.show()
sys.exit(app.exec_())
