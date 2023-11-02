import sys
#import random
from PyQt5.QtWidgets import QApplication, QWidget, QPushButton, QVBoxLayout, QGraphicsScene, QGraphicsView, QGraphicsEllipseItem, QGraphicsLineItem
from PyQt5.QtCore import Qt, QTimer
from random import randint

class MyWindow(QWidget):
    def __init__(self):
        super().__init__()

        self.initUI()

    def initUI(self):
        self.setGeometry(100, 100, 800, 600)
        self.setWindowTitle('Camera Test App')

        self.scene = QGraphicsScene(self)
        self.view = QGraphicsView(self.scene)

        self.button = QPushButton('Test Camera', self)
        self.button.clicked.connect(self.test_camera)

        layout = QVBoxLayout()
        layout.addWidget(self.view)
        layout.addWidget(self.button)

        self.setLayout(layout)

    def test_camera(self):
        self.timer = QTimer(self)
        self.timer.timeout.connect(self.update_display)
        self.timer.start(200)  # 500 milliseconds (0.5 seconds) interval
        self.timer_count = 0

    def update_display(self):
        if self.timer_count >= 25:  # 10 iterations, 0.5 seconds each, total 5 seconds
            self.timer.stop()
            self.scene.clear()
            return

        #test_list = [random.randint(300, 600) for _ in range(12)]
        test_list = [randint(250,350),randint(250,350),randint(550,650),randint(250,350),randint(250,350),randint(550,650),randint(550,650),randint(550,650),randint(400,450),randint(400,450),randint(400,450),randint(400,450)]

        # Clear previous items on the scene
        self.scene.clear()

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

if __name__ == '__main__':
    app = QApplication(sys.argv)
    window = MyWindow()
    window.show()
    sys.exit(app.exec_())
