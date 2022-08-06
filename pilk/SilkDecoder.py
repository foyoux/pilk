"""silk 解码"""
from typing_extensions import Literal

from ._pilk import decode

PCM_RATE = Literal[
    # pcm data rate 可选值
    8000, 12000, 16000, 24000, 32000, 44100, 48000
]

# Uplink loss estimate, in percent (0-100); default: 0
PACKET_LOSS = int


class SilkDecoder:
    """..."""

    def __init__(self, pcm_rate: PCM_RATE = 24000, packet_loss: PACKET_LOSS = 0):
        self.pcm_rate = pcm_rate
        self.packet_loss = packet_loss

    def decode(self, silk: str, pcm: str) -> int:
        return decode(
            silk, pcm,
            pcm_rate=self.pcm_rate, packet_loss=self.packet_loss
        )
