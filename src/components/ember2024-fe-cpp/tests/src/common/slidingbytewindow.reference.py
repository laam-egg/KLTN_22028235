# This code is from the original Python implementation,
# added here for reference (e.g. to generate examples
# in test cases).

import numpy as np

def stride_trick(bytez, window, step):
    a = np.frombuffer(bytez, dtype=np.uint8)
    shape = a.shape[:-1] + (a.shape[-1] - window + 1, window)
    strides = a.strides + (a.strides[-1],)
    blocks = np.lib.stride_tricks.as_strided(a, shape=shape, strides=strides)[:: step, :]
    return blocks

if __name__ == "__main__":
    bytez = b'abcdefghij'
    window = 4
    step = 2
    result = stride_trick(bytez, window, step)
    print(result)
    # Output the result as a list of byte strings for better readability
    # byte_strings = [bytes(block) for block in result]
    # print(byte_strings)
