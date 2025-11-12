import matplotlib.pyplot as plt
import numpy as np


# Чтение файла в формате CS16
data = np.fromfile('build/txdata.pcm', dtype=np.int16)

# Преобразование в комплексные числа и нормализация
iq_data = data.reshape(-1, 2)  # Разделяем на I и Q
complex_signal = iq_data[:, 0] + 1j * iq_data[:, 1]
#normalized_signal = complex_signal / 32768.0  # Нормализация для [-1, 1]

# Визуализация
plt.figure()

plt.plot(complex_signal.real, label='I')
plt.plot(complex_signal.imag, label='Q')
plt.title('I and Q Components (first 1000 samples)')
plt.xlabel('Sample Index')
plt.ylabel('Amplitude')
plt.legend()
plt.grid(True)

plt.tight_layout()
plt.show()