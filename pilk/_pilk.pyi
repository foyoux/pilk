def encode(
        pcm: str,
        silk: str = None,
        pcm_rate: int = 24000,
        silk_rate: int = None,
        tencent: bool = False,
        max_rate: int = 24000,
        complexity: int = 2,
        packet_size: int = 20,
        packet_loss: int = 0,
        use_in_band_fec: bool = False,
        use_dtx: bool = False
) -> tuple[str, int]:
    """silk 编码

    :param pcm: str: pcm 格式文件路径
    :param silk: str[可选]: 编码完成的 silk 文件路径, 默认为 `pcm`参数 + '.silk'
    :param pcm_rate: int(hz)[可选]: 输入 pcm 文件的 rate, 默认: 24000. 可选值为 [8000, 12000, 16000, 24000, 32000, 44100, 48000]
    :param silk_rate: int(8000~48000 hz)[可选]: 默认: 与 pcm_rate 一致,输出 silk 文件的 rate
    :param tencent: bool[可选]: 默认: False, 是否兼容腾讯系语音
    :param max_rate: int[可选]: 默认: 24000, 最大内部采样率, 可选值为 [8000, 12000, 16000, 24000]
    :param complexity: int[可选]: Set complexity, 0: low, 1: medium, 2: high; default: 2
    :param packet_size: Packet interval in ms, default: 20
    :param packet_loss: Uplink loss estimate, in percent (0-100); default: 0
    :param use_in_band_fec: Enable inband FEC usage (0/1); default: 0
    :param use_dtx: Enable DTX (0/1); default: 0
    :return: (<silk文件路径>, <silk文件持续时间>)
    """


def decode(
        silk: str,
        pcm: str = None,
        pcm_rate: int = 24000,
        packet_loss: int = 0
) -> tuple[str, int]:
    """silk 解码

    :param silk: str： silk 输入文件路径
    :param pcm: str[可选]： pcm 输出文件路径, 未提供则使用 `silk` 参数 + '.pcm'
    :param pcm_rate: int(hz)[可选]: 输入 pcm 文件的 rate, 默认: 24000. 可选值为 [8000, 12000, 16000, 24000, 32000, 44100, 48000]
    :param packet_loss: Uplink loss estimate, in percent (0-100); default: 0
    :return: (<pcm 文件路径>, <pcm 文件时间>)
    """
