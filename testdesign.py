import sys
#import random
from PyQt5.QtWidgets import QComboBox, QApplication, QWidget, QPushButton, QVBoxLayout, QGraphicsScene, QGraphicsView, QGraphicsEllipseItem, QGraphicsLineItem
from PyQt5.QtCore import Qt, QTimer
from random import randint
import serial
import serial.tools.list_ports

class MyWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.setGeometry(100, 100, 1023, 768)
        self.setWindowTitle('Camera Test App')

        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene)

        self.listbutton = QPushButton('Detect Guns', self)
        self.listbutton.clicked.connect(self.list_guns)

        self.listbox = QComboBox()

        self.solbutton = QPushButton('Test Solenoid', self)
        self.solbutton.clicked.connect(self.test_sol)



        self.cambutton = QPushButton('Test Camera', self)
        self.cambutton.clicked.connect(self.test_camera)

        self.rumbutton = QPushButton('Test Rumble', self)
        self.rumbutton.clicked.connect(self.test_rum)


        layout = QVBoxLayout()
        layout.addWidget(self.view)
        layout.addWidget(self.listbutton)
        layout.addWidget(self.listbox)
        layout.addWidget(self.solbutton)
        layout.addWidget(self.cambutton)
        layout.addWidget(self.rumbutton)

        self.setLayout(layout)

    def test_camera(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_display)
        self.timer.start(30)  # 500 milliseconds (0.5 seconds) interval
        self.timer_count = 0

    def update_display(self):
        if self.timer_count >= 150:  # 10 iterations, 0.5 seconds each, total 5 seconds
            self.timer.stop()
            self.scene.clear()
            print("camera test done")
            return

        #test_list = [random.randint(300, 600) for _ in range(12)]
        #test_list = [randint(250,350),randint(250,350),randint(550,650),randint(250,350),randint(250,350),randint(550,650),randint(550,650),randint(550,650),randint(400,450),randint(400,450),randint(400,450),randint(400,450)]

        # Clear previous items on the scene
        self.scene.clear()

        self.camStr = "cam"

        self.ser = serial.Serial(self.listbox.currentText(), 115200)

        self.ser.write(self.camStr.encode('ascii'))
        self.line = self.ser.readline().decode('utf-8').rstrip()
        self.ser.close()
        self.my_list = self.line.split(",")
        test_list = [eval(i) for i in self.my_list]
        print(test_list)

        # Draw Ellipses
        for i in range(0, 12, 2):
            ellipse = QGraphicsEllipseItem(test_list[i], test_list[i + 1], 35, 35)
            self.scene.addItem(ellipse)

        # Draw Lines
        self.scene.addItem(QGraphicsLineItem(test_list[0], test_list[1], test_list[2], test_list[3]))
        self.scene.addItem(QGraphicsLineItem(test_list[0], test_list[1], test_list[4], test_list[5]))
        self.scene.addItem(QGraphicsLineItem(test_list[4], test_list[5], test_list[6], test_list[7]))
        self.scene.addItem(QGraphicsLineItem(test_list[2], test_list[3], test_list[6], test_list[7]))

        self.timer_count += 1

    def list_guns(self):
        print("List serial connections: ")
        self.ports = serial.tools.list_ports.comports()
        self.listbox.clear()
        for p in self.ports:
            print(p.device)
            self.listbox.addItem(p.device)
        print(self.listbox.currentText())

    def test_rum(self):
        print("testing rumble")
        self.ser = serial.Serial(self.listbox.currentText(), 115200)
        self.rumStr = "rum"

        if self.ser.isOpen():
            self.ser.write(self.rumStr.encode('ascii'))
        self.ser.close()

    def test_sol(self):
        print("testing solenoid")
        self.ser = serial.Serial(self.listbox.currentText(), 115200)
        self.solStr = "sol"

        if self.ser.isOpen():
            self.ser.write(self.solStr.encode('ascii'))
        self.ser.close()


if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MyWindow()
    window.show()
    sys.exit(app.exec_())
