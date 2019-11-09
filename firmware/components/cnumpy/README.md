# cnumpy

cnumpy is a ANSI C library that allows the ability to read and write .npy files. npz files are not supported as the gzip compression in an npz file only supports block compression. This would make it unable to support low ram utilisation operations sunch as chunked operations or streaming applications.

# file format

![npy file](assets/npy.png)

The .npy file format can be found here: https://numpy.org/devdocs/reference/generated/numpy.lib.format.html

# OpenHAP application

The npy file format is useful for storing thermal image data. It may also b extended to other data types with fixed/predetermined size.
