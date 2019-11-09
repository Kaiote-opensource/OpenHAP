# cnumpy

cnumpy is a derivative work on c2numpy by Jim Pivarski . cnumpy carries over the original Apache licence used.

cnumpy is an ANSI C compatible library that allows the ability to read and write .npy files. npz files are not supported as hey use gzip compression which only supports block compression. This would make it unable to support low ram utilisation operations sunch as chunked by chunk data operations.

# file format

![npy file](assets/npy.png)

The .npy file format can be found here: https://numpy.org/devdocs/reference/generated/numpy.lib.format.html

# OpenHAP application

The npy file format is useful for storing thermal image data. It may also be extended to other data types with fixed/predetermined size.
