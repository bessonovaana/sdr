import numpy as np
import librosa
from pydub import AudioSegment

def mp3_to_pcm(mp3_file, pcm_file):
    y, sr = librosa.load(mp3_file, sr=44100, mono=True)
    pcm_data = (y * 32767).astype(np.int16)
    pcm_data.tofile(pcm_file)

def pcm_to_mp3(pcm_file,mp3_file):
    pcm_data = np.fromfile(pcm_file, dtype=np.int16)
    audio = AudioSegment(
    data=pcm_data.tobytes(),
    sample_width=2,      # 2 байта = 16 бит
    frame_rate=44100,    # частота дискретизации
    channels=1           # моно
    )
    audio.export(mp3_file, format="mp3", bitrate="192k")
if __name__ == '__main__':
    mp3_to_pcm("/home/anastasia/Рабочий стол/sdr/pract/dev/Smeshariki.mp3", "music1.pcm")
    pcm_to_mp3("build/txdata.pcm", "music2.mp3")
