import numpy as np
import matplotlib.pyplot as plt
import os

# Открываем файл для чтения
names = ["../tx.pcm", "../rx.pcm"]
for i,name in enumerate(names):
    data = []
    imag = []
    real = []
    count = []
    counter = 0
    with open(name, "rb") as f:
        index = 0
        while (byte := f.read(2)):
            if(index %2 == 0):
                real.append(int.from_bytes(byte, byteorder='little', signed=True))
                counter += 1
                count.append(counter)
            else:
                imag.append(int.from_bytes(byte, byteorder='little', signed=True))
            index += 1



    #fig, axs = plt.subplots(2, 1, layout='constrained')
    plt.figure(1)
    plt.subplot(4,1,2*i+1)
    #axs[1].plot(count, np.abs(data),  color='grey')  # Используем scatter для диаграммы созвездия
    plt.plot(count,(imag),color='red', label="q")  # Используем scatter для диаграммы созвездия
    plt.plot(count,(real), color='blue', label="i")  # Используем scatter для диаграммы созвездия
    plt.legend()
    plt.grid()
    plt.subplot(4,1,2*i+2)
    plt.scatter((real),(imag))
    plt.grid()
    plt.tight_layout()

plt.show()