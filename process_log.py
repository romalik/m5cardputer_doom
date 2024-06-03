

import sys
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.widgets import Button

refs_by_frame = {}
memmap = {}


def read_memmap(fname):
    file = open(fname, 'r')
    file_lines = file.readlines()
    file.close()
    for line in file_lines:
        vals = line.split()
        key = vals[1].lstrip("0")
        if key == "":
            continue

        memmap[int(key)] = vals[3]


def read_log(fname):
    file = open(fname,'r')
    file_lines = file.readlines()
    file.close()

    
    for line in file_lines:
        if line.startswith("ACCESS_AT"):
            vals = line.split()
            refs_by_frame[int(vals[1])] = list(map(int, vals[2:]))

    

    #print(refs_by_frame)


def plot_frame(ax, frame):
    pages = refs_by_frame[frame][0::2]
    nrefs = refs_by_frame[frame][1::2]
 
    pages, nrefs = zip(*sorted(zip(pages, nrefs)))
    ax.scatter(x=list(pages), y=list(nrefs), c="r")
    
    text = "START"
    
    annotated = []

    for i in range(len(pages)):
        for page_start in memmap:
            if page_start > pages[i]:
                break
            text = memmap[page_start]

        if text not in annotated:
            ax.annotate(text, (pages[i], nrefs[i]))
            annotated.append(text)


frame = 1

def plot_log():
    global frame
    


    max_page = max(map(max,((v[0::2] for v in refs_by_frame.values()))))
    max_nref = max(map(max,((v[1::2] for v in refs_by_frame.values()))))

    fig = plt.figure()
    ax = fig.add_subplot(111)

    ax.set_xlim([0, max_page])
    ax.set_ylim([0, max_nref])

    fig.subplots_adjust(left=0.25, bottom=0.25)

    plot_frame(ax, frame)

    next_button_ax = fig.add_axes([0.8, 0.025, 0.1, 0.04])
    next_button = Button(next_button_ax, 'Next', hovercolor='0.975')
    def next_button_on_clicked(mouse_event):
        global frame
        if(frame < len(refs_by_frame)):
            frame = frame + 1

        ax.clear()
        ax.set_xlim([0, max_page])
        ax.set_ylim([0, max_nref])

        plot_frame(ax, frame)
        fig.canvas.draw_idle()

    next_button.on_clicked(next_button_on_clicked)

    prev_button_ax = fig.add_axes([0.2, 0.025, 0.1, 0.04])
    prev_button = Button(prev_button_ax, 'Prev', hovercolor='0.975')
    def prev_button_on_clicked(mouse_event):
        global frame
        if(frame > 1):
            frame = frame - 1

        ax.clear()
        ax.set_xlim([0, max_page])
        ax.set_ylim([0, max_nref])

        plot_frame(ax, frame)
        fig.canvas.draw_idle()

    prev_button.on_clicked(prev_button_on_clicked)


    plt.show()


def main():
    if len(sys.argv) < 2:
        exit()

    if len(sys.argv) == 3:
        read_memmap(sys.argv[2])

    read_log(sys.argv[1])
    plot_log()

if __name__ == '__main__':
    main()