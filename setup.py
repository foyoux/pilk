from glob import glob

from setuptools import setup, Extension

SKP_SILK_SRC = 'src/SKP_SILK_SRC/'
sources = glob(SKP_SILK_SRC + '*.c') + glob('src/silk/*.c')
# noinspection SpellCheckingInspection
sources.append('src/pilkmodule.c')

# noinspection SpellCheckingInspection
pilkmodule = Extension(
    name='pilk._pilk',
    sources=sources,
    include_dirs=[SKP_SILK_SRC, 'src/interface', 'src/silk']
)

with open('README.md', encoding='utf8') as f:
    long_description = f.read()

# noinspection SpellCheckingInspection
setup(
    name='pilk',
    version='0.2.4',
    description='python silk voice library',
    long_description=long_description,
    long_description_content_type='text/markdown',
    author='foyou',
    author_email='yimi.0822@qq.com',
    maintainer='foyou',
    maintainer_email='yimi.0822@qq.com',
    url='https://github.com/foyoux/pilk',
    download_url='https://github.com/foyoux/pilk/releases',
    license_files=['LICENSE'],
    keywords=['silk', 'voice', 'python', 'extension', 'wechat', 'qq', 'tencent', 'xposed', 'c/c++'],
    python_requires='>=3.6',
    ext_modules=[pilkmodule],
    zip_safe=False,
    packages=['pilk'],
    package_data={
        'pilk': ['_pilk.pyi']
    },
    install_requires=['typing_extensions;python_version<"3.8"'],
    classifiers=[
        'License :: OSI Approved :: GNU General Public License v3 (GPLv3)',
        'Operating System :: Microsoft :: Windows',
        'Operating System :: POSIX',
        'Programming Language :: C',
        'Programming Language :: Python',
        'Programming Language :: Python :: 3',
        'Programming Language :: Python :: 3.6',
        'Programming Language :: Python :: 3.7',
        'Programming Language :: Python :: 3.8',
        'Programming Language :: Python :: 3.9',
        'Programming Language :: Python :: 3.10',
        'Programming Language :: Python :: 3.11',
        'Programming Language :: Python :: 3.12',
        'Topic :: Multimedia :: Sound/Audio'
    ],
)
