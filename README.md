# pilk

python silk codec binding 支持微信语音编解码

pilk: python + silk

## 安装

[![python version](https://img.shields.io/pypi/pyversions/pilk)](https://pypi.org/project/aligo/)  [![downloads](https://static.pepy.tech/personalized-badge/pilk?period=total&units=international_system&left_color=black&right_color=orange&left_text=Downloads)](https://pepy.tech/project/pilk)

```bash
pip install pilk
```

## 介绍与说明

[**SILK**](https://en.wikipedia.org/wiki/SILK) 是一种语音编码格式，由 [**Skype**](https://en.wikipedia.org/wiki/Skype_Technologies)
公司研发，网上可找到的最新版本是 2012 发布的。

**SILK** 原始代码已上传到 [v0.0.1 release](https://github.com/foyoux/pilk/releases/tag/v0.0.1) , 包含规范文档

**Tencent** 系语音支持来自 [silk-v3-decoder](https://github.com/kn007/silk-v3-decoder)

[v0.0.1 release ](https://github.com/foyoux/pilk/releases/tag/v0.0.1)
中也包含 [silk-v3-decoder](https://github.com/kn007/silk-v3-decoder) 重编译的 **x64-win** 版本

### **SILK** 编码格式 和 **Tencent** 系语音的关系：

> 此处 **Tencent** 系语音，仅以微信语音为例

1. 标准 **SILK** 文件以 `b'#!SILK_V3'` 开始，以 `b'\xFF\xFF'` 结束，中间为语音数据
2. 微信语音文件在标准 **SILK** 文件的开头插入了 `b'\x02'`，去除了结尾的 `b'\xFF\xFF'`，中间不变

> 已下统称为语音文件

### 语音数据

语音数据分为很多个独立 **frame**，每个 **frame** 开头两字节存储剩余 **frame** 数据的大小，每个 **frame** 默认存储 **20ms** 的音频数据

据此可写出获取 **语音文件** 持续时间(duration) 的函数（此函数 **pilk** 中已包含）

```python
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
            size = size[0] + size[1] << 8
            if not tencent and size == 0xffff:
                break
            i += 1
            silk.seek(silk.tell() + size)
        return i * frame_ms
```

根据 **SILK** 格式规范，**frame_ms** 可为 `20, 40, 60, 80, 100`

## 快速入门

> 详情请在 IDE 中查看 API 文档注释

在使用 **pilk** 之前，你还需清楚 **音频文件 `mp3, aac, m4a, flac, wav, ...`** 与 **语音文件** 之间的转换是借助 [**PCM raw
data**](https://en.wikipedia.org/wiki/Pulse-code_modulation) 完成的

具体转换关系：音频文件 ⇔ PCM ⇔ 语音文件

1. 音(视)频文件 ➜ PCM
   > 借助 ffmpeg，你当然需要先有 [ffmpeg](https://www.ffmpeg.org/download.html)
    ```bat
    ffmpeg -y -i <音(视)频输入文件> -vn -ar <采样率> -ac 1 -f s16le <PCM输出文件>
    ```
    1. `-y`: 可加可不加，表示 <PCM输出文件> 已存在时不询问，直接覆盖
    2. `-i`: 没啥好说的，固定的，后接 <音(视)频输入文件>
    3. `-vn`: 表示不处理视频数据，建议添加，虽然不加也不会处理视频数据（视频数据不存在转PCM的说法），但可能会打印警告
    4. `-ar`: 设置采样率，可选的值是 [8000, 12000, 16000, 24000, 32000, 44100, 48000], 这里你可以直接理解为声音质量
    5. `-ac`: 设置声道数，在这里必须为 **1**，这是由 **SILK** 决定的
    6. `-f`: 表示强制转换为指定的格式，一般来说必须为 **s16le**, 表示 `16-bit short integer Little-Endian data`
    7. example1: `ffmpeg -y -i mv.mp4 -vn -ar 44100 -ac 1 -f s16le mv.pcm`
    8. example2: `ffmpeg -y -i music.mp3 -ar 44100 -ac 1 -f s16le music.pcm`


2. PCM ➜ 音频文件
    ```bat
    ffmpeg -y -f s16le -i <PCM输入文件> -ar <采样率> -ac <声道数> <音频输出文件>
    ```
    1. `-f`: 这里必须为 `s16le`, 同样也是由 **SILK** 决定的
    2. `-ar`: 同上
    3. `-ac`: 含义同上，值随意
    4. `<音频输出文件>`: 扩展名要准确，没有指定格式时，**ffmpeg** 会根据给定的输出文件扩展名来判断需要输出的格式
    5. example3: `ffmpeg -y -f s16le -i test.pcm test.mp3`

讲完了 音频文件 ⇔ PCM，接下来就是用 **pilk** 进行 PCM ⇔ 语音文件 互转

### silk 编码

```python
import pilk

# pcm_rate 参数必须和 使用 ffmpeg 转 音频 到 PCM 文件时，使用的 `-ar` 参数一致
# pcm_rate 参数必须和 使用 ffmpeg 转 音频 到 PCM 文件时，使用的 `-ar` 参数一致
# pcm_rate 参数必须和 使用 ffmpeg 转 音频 到 PCM 文件时，使用的 `-ar` 参数一致
duration = pilk.encode("test.pcm", "test.silk", pcm_rate=44100, tencent=True)

print("语音时间为:", duration)
```

### silk 解码

```python
import pilk

# pcm_rate 参数必须和 使用 ffmpeg 转 音频 到 PCM 文件时，使用的 `-ar` 参数一致
duration = pilk.decode("test.silk", "test.pcm")

print("语音时间为:", duration)
```
