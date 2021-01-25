from tkinter import *
from tkinter import filedialog
import os

IPC_FIFO_NAME = "client_pipe"
IPC_FIFO_NAME2 = "gui_pipe"
writefifo = os.open(IPC_FIFO_NAME, os.O_WRONLY)

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


def make_req(rtype, bucket=None, filename=None):
    msg = str(rnum[rtype])

    if bucket != None:
        arr = [(bucket >> 8) & 0xFF, bucket & 0xFF]
        msg += chr(arr[0]) + chr(arr[1])

    if filename != None:
        if len(filename) > 200:
            return None
        msg += filename

    return msg.encode("ascii")


def send_req(request):
    os.write(writefifo, bytes(request))


def read_pipe():
    data = []
    acum = ""
    with open(IPC_FIFO_NAME2) as readf:
        while True:
            last = readf.read()

            if len(last) == 0:
                print("no more input for GUI")
                break

            last = last.strip().split('$')
            if(last[-1] == ''): last.pop()
            print('last',last)            
            data.extend(last);

    return data


class tk_wrapper:
    def __init__(self, tk):
        self.tk = tk
        self.tk.winfo_toplevel().title("SCHERZO")

    def main_loop(self):
        while True:
            try:
                self.tk.update_idletasks()
                self.tk.update()
            except:
                req = make_req(1, 69, "69")
                send_req(req);



class gui_panel:
    def __init__(self, root, parent=None, w=800, h=800):
        self.root = root
        self.root.tk.configure(background="#999999")
        self.frame = Frame(self.root.tk, width=w, height=h)
        self.frame.configure(background="#999999")
        self.parent = parent
        self.on_error = False
        self.add_widgets()

    def add_widgets(self):
        pass
    
    def make_visible(self):
        self.frame.pack()   

    def change_to(self, other):
        if not self.on_error:
            other.parent = self
            other.make_visible()
            self.frame.pack_forget()

    def to_parent(self):
        if not self.on_error:
            self.parent.make_visible()
            self.parent = None
            self.frame.pack_forget()

    def get_rootX(self):
        return self.root.tk.winfo_width()

    def get_rootY(self):
        return self.root.tk.winfo_height()


class files_panel(gui_panel):
    def __init__(self, root, bucket, w=800, h=800):
        gui_panel.__init__(self, root, w=w, h=h)
        self.bucket = bucket

    def get_data(self):
        # query bucket here
        req = make_req("LIST", self.bucket)
        if req == None:
            print("malformed request")
            return None
        else:
            send_req(req)
            data = read_pipe()
            return data

        return ["66", "66", "66"]

    def make_visible(self):
        print(self.bucket)
        self.files.delete(0, END)
        data = self.get_data()
        for d in data:
            self.files.insert(END, d)
        
        self.frame.pack()

    def new_file(self):
        filename = filedialog.askopenfilename(
            initialdir="~", title="Seleccionar Archivo"
        )

        filename = filename.split("/")
        filename = filename[-1]

        req = make_req("RECV", self.bucket, filename)
        if req == None:
            print("malformed request")
        else:
            send_req(req)
            self.files.insert(END, filename)

    def del_file(self):
        filename = self.files.get(ACTIVE)
        req = make_req("DELF", self.bucket, filename)
        if req == None:
            print("malformed request")
        else:
            send_req(req)
            self.files.delete(ACTIVE)

    def get_file(self):
        filename = self.files.get(ACTIVE)
        req = make_req("SEND", self.bucket, filename)
        if req == None:
            print("malformed request")
        else:
            send_req(req)

    def add_widgets(self):

        addf = Button(
            self.frame,
            text="Agregar Archivo",
            command=self.new_file,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        delf = Button(
            self.frame,
            text="Borrar Archivo",
            command=self.del_file,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        getf = Button(
            self.frame,
            text="Obtener Archivo",
            command=self.get_file,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        goback = Button(
            self.frame,
            text="Volver a buckets",
            command=self.to_parent,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        title = Label(
            self.frame, text="Scherzo-Server", font=("Sans-Serif", "12", "bold")
        )

        self.files = Listbox(
            self.frame, width=40, height=40, font=("Sans-Serif", "12", "bold")
        )

        addf.grid(row=3, column=0)
        delf.grid(row=3, column=0, pady=(100, 0))
        getf.grid(row=3, column=0, pady=(200, 0))
        goback.grid(row=3, column=0, pady=(300, 0))
        title.grid(row=0, column=3, padx=(40, 0), pady=20)
        self.files.grid(row=3, column=3, padx=(50, 0), pady=(20, 0))


class buck_panel(gui_panel):
    def __init__(self, root, fpanel, w=800, h=800):
        gui_panel.__init__(self, root, w=w, h=h)
        self.fpanel = fpanel
        self.last = 0

    def get_data(self):
        req = make_req("LISTB")
        if req == None:
            print("malformed request")
            return None
        else:
            send_req(req)
            data = read_pipe()
            if len(data) > 0:
                self.last = int(data[-1])
            return data

    def make_visible(self):
        self.bucks.delete(0, END)
        data = self.get_data()
        for d in data:
            self.bucks.insert(END, d)
        if len(data) > 0:            
            self.last = int(data[-1]);

        self.frame.pack()

    def goto_file_panel(self):
        self.fpanel.bucket = int(self.bucks.get(ACTIVE))
        self.change_to(self.fpanel)

    def add_bucket(self):
        self.last += 1
        req = make_req("NEWB", self.last)
        if req == None:
            print("malformed request")
        else:
            send_req(req)
            self.bucks.insert(END, self.last)

    def del_bucket(self):
        buck = int(self.bucks.get(ACTIVE))
        req = make_req("DELB", buck)
        if req == None:
            print("malformed request")
        else:
            send_req(req)
            self.bucks.delete(ACTIVE)

    def add_widgets(self):
        gtfiles = Button(
            self.frame,
            text="Ver archivos",
            command=self.goto_file_panel,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        addb = Button(
            self.frame,
            text="Nuevo Bucket",
            command=self.add_bucket,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        delb = Button(
            self.frame,
            text="Borrar Bucket",
            command=self.del_bucket,
            bg="#ffffff",
            relief=RIDGE,
            font=("Sans-Serif", "12", "bold"),
        )

        title = Label(
            self.frame, text="Scherzo-Server", font=("Sans-Serif", "12", "bold")
        )

        self.bucks = Listbox(
            self.frame, width=40, height=40, font=("Sans-Serif", "12", "bold")
        )

        addb.grid(row=3, column=0)
        delb.grid(row=3, column=0, pady=(100, 0))
        gtfiles.grid(row=3, column=0, pady=(200, 0))
        title.grid(row=0, column=3, padx=(40, 0), pady=20)
        self.bucks.grid(row=3, column=3, padx=(50, 0), pady=(20, 0))


def create_panels(root):
    w = root.tk.winfo_width()
    h = root.tk.winfo_height()

    files = files_panel(root, 0)
    buckets = buck_panel(root, files)

    buckets.make_visible()


def main():
    root = Tk()
    root.geometry("900x700")
    main = tk_wrapper(root)
    print("goodbye")
    create_panels(main)
    print("goodbye")
    main.main_loop()


main()
