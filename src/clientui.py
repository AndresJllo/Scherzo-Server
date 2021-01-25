import os
import struct
import subprocess

rnum = {
    "OK": 0,
    "ERR": 1,
    "DELF": 2,
    "SEND": 3,
    "RECV": 4,
    "LIST": 5,
    "DELB": 6,
    "NEWB": 7,
    "LISTB": 8,
}


def parse_input():
    rtype = input("REQUEST TYPE: ").strip()
    if rtype not in rnum:
        return None

    if rtype == "LISTB":
        msg = str(rnum[rtype])
        return msg.encode("ascii")

    bucket = int(input("BUCKET: ").strip())
    if bucket < 0 or bucket > (1 << 16):
        return None

    filename = ""

    if rtype == "SEND" or rtype == "RECV" or rtype == "DELF":
        filename = input("FILENAME: ").strip()

    if len(filename) > 200:
        return None

    arr = [(bucket >> 8) & 0xFF, bucket & 0xFF]

    msg = str(rnum[rtype]) + chr(arr[0]) + chr(arr[1]) + filename
    return msg.encode("ascii")


if __name__ == "__main__":

    IPC_FIFO_NAME = "client_pipe"
    # subprocess.call("./client")
    fifo = os.open(IPC_FIFO_NAME, os.O_WRONLY)
    try:
        while True:
            request = parse_input()
            if request == None:
                print("malformed request")
            else:
                os.write(fifo, bytes(request))
    except KeyboardInterrupt:
        print("\nbye!")
    finally:
        os.close(fifo)
