"""silk 编码器"""
from typing import Literal

from ._pilk import encode

PCM_RATE = Literal[
    # pcm data rate 可选值
    8000, 12000, 16000, 24000, 32000, 44100, 48000
]

MAX_RATE = Literal[
    # 最大内部采样率
    8000, 12000, 16000, 24000  # default: 24000
]

COMPLEXITY = Literal[
    # Set complexity
    0,  # low
    1,  # medium
    2,  # high (default)
]

PACKET_SIZE = Literal[
    # Packet interval in ms
    20, 40, 60, 80, 100
]

# 8000 ~ 48000 HZ
SILK_RATE = int

# Uplink loss estimate, in percent (0-100); default: 0
PACKET_LOSS = int


class SilkEncoder:
    """..."""

    def __init__(
            self,
            pcm_rate: PCM_RATE = 24000,
            silk_rate: SILK_RATE = None,
            max_rate: MAX_RATE = 24000,
            complexity: COMPLEXITY = 2,
            packet_size: PACKET_SIZE = 20,
            packet_loss: PACKET_LOSS = 0,
            use_in_band_fec: bool = False,
            use_dtx: bool = False,
    ):
        self.pcm_rate = pcm_rate
        self.silk_rate = silk_rate
        self.max_rate = max_rate
        self.complexity = complexity
        self.packet_size = packet_size
        self.packet_loss = packet_loss
        self.use_in_band_fec = use_in_band_fec
        self.use_dtx = use_dtx

    def encode(self, pcm: str, silk: str, tencent: bool = False) -> int:
        return encode(
            pcm, silk,
            self.pcm_rate,
            self.silk_rate,
            tencent,
            self.max_rate,
            self.complexity,
            self.packet_size,
            self.packet_loss,
            self.use_in_band_fec,
            self.use_dtx,
        )
