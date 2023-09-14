import tempfile
import wave

from .SilkDecoder import SilkDecoder
from .SilkEncoder import SilkEncoder
from ._pilk import *

__title__ = 'pilk'
__description__ = 'python silk codec binding'
__url__ = 'https://github.com/foyoux/pilk'
__version__ = '0.2.4'
# noinspection SpellCheckingInspection
__author__ = 'foyou'
__author_email__ = 'yimi.0822@qq.com'
__license__ = 'GPL-3.0'
__copyright__ = f'Copyright 2022~2023 {__author__}'
__ide__ = 'PyCharm - https://www.jetbrains.com/pycharm/'


def get_duration(silk_path: str, frame_ms: int = 20) -> int:
    """获取 silk 文件持续时间，单位：ms"""
    with open(silk_path, 'rb') as silk:
        tencent = False
        if silk.read(1) == b'\x02':
            tencent = True
        silk.seek(0)
        if tencent:
            silk.seek(10)
        else:
            silk.seek(9)
        i = 0
        while True:
            size = silk.read(2)
            if len(size) != 2:
                break
            i += 1
            size = size[0] + size[1] * 16
            silk.seek(silk.tell() + size)
        return i * frame_ms


def silk_to_wav(silk: str, wav: str, rate: int = 24000):
    """silk 文件转 wav"""
    pcm_path = tempfile.mktemp(suffix='.pcm')
    decode(silk, pcm_path, pcm_rate=rate)
    with open(pcm_path, 'rb') as pcm:
        with wave.open(wav, 'wb') as f:
            # noinspection PyTypeChecker
            f.setparams((1, 2, rate, 0, 'NONE', 'NONE'))
            f.writeframes(pcm.read())
