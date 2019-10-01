# The following program is a Python 3.5 GUI application written by Matthew Niemiec intended to help
# students (in college) keep track of their grades. The project was started in the fall 2015 semester
# and this version is being finished in the summer of 2016. The idea of the program is to store grades
# and class preferences, as well as make the somewhat tedious calculations of entering class weights.
# Of course, the user is welcome to use it however they please, but it is designed to be updated throughout
# the course of the semester. As far as code is concerned, essentially the whole script is written
# inside a class, as is the standard for object-oriented Python Programming. Please email any comments
# or suggestions to pivotman624@gmail.com.

# Import statements
from tkinter import *
from tkinter import messagebox
from tkinter.filedialog import askopenfilename
from threading import Thread
import shutil
import csv
import os
import subprocess
import pickle


# The following is code imported from http://www.voidspace.org.uk/python/weblog/arch_d7_2006_07_01.shtml
# This is the code I use to create a tooltip for all of the options and help
class ToolTip(object):

    def __init__(self, widget):
        self.widget = widget
        self.tipwindow = None
        self.id = None
        self.x = self.y = 0

    def showtip(self, text):
        "Display text in tooltip window"
        self.text = text
        if self.tipwindow or not self.text:
            return
        x, y, cx, cy = self.widget.bbox("insert")
        x = x + self.widget.winfo_rootx() + 27
        y = y + cy + self.widget.winfo_rooty() +27
        self.tipwindow = tw = Toplevel(self.widget)
        tw.wm_overrideredirect(1)
        tw.wm_geometry("+%d+%d" % (x, y))
        try:
            # For Mac OS
            tw.tk.call("::tk::unsupported::MacWindowStyle",
                       "style", tw._w,
                       "help", "noActivates")
        except TclError:
            pass
        label = Label(tw, text=self.text, justify=LEFT,
                      background="#ffffe0", relief=SOLID, borderwidth=1,
                      font=("tahoma", "8", "normal"))
        label.pack(ipadx=1)

    def hidetip(self):
        tw = self.tipwindow
        self.tipwindow = None
        if tw:
            tw.destroy()


def createToolTip(widget, text):
    toolTip = ToolTip(widget)

    def enter(event):
        toolTip.showtip(text)

    def leave(event):
        toolTip.hidetip()
    widget.bind('<Enter>', enter)
    widget.bind('<Leave>', leave)
# This is the end of my imported toop tip code. The rest is written by me


# This is the class used to contain the application. All functionality and design are stored inside the
# following lines of code
class Gui:
    # Due to the nature of the program, the interface needs to restart to show different classes.
    # Thus, these are the values that the program is given when the user first opens the application
    # before loading a class. The rest of the values are stored in __init__.

    # These variables set up the physical layout of the main root.
    active_boxes = 6
    root = Tk()
    root.title("The World's Best Grade Averager!")
    img = PhotoImage(file='favicon.gif')
    root.tk.call('wm', 'iconphoto', root._w, img)

    # These variables make up some of the more technical aspects of the program
    class_info = ""
    save_state = "disabled"
    categories = 0
    class_index = -1
    line = []
    title_extension = ""
    status_fill = ""

    # photo = PhotoImage(file="Instruction.png")

    # Options dealing with the Show section of the Customize menu
    show_file_name = "show.pkl"
    try:
        read_show = open(show_file_name, 'rb')
        show_states = pickle.load(read_show)
        read_show.close()
    except FileNotFoundError:
        show_states = [0, 1]
        write_show = open(show_file_name, 'wb')
        pickle.dump(show_states, write_show, pickle.HIGHEST_PROTOCOL)
        write_show.close()
    show_drop = BooleanVar()
    show_extra_credit = BooleanVar()
    show_drop.set(show_states[0])
    show_extra_credit.set(show_states[1])

    if show_states[1]:
        farthest_left = 4
    elif show_states[0]:
        farthest_left = 3
    else:
        farthest_left = 2

    # This lets the program know that the user is not trying to rename a class, add a class, get help, etc.
    child_window_open = False

    def __init__(self):
        # Multithreading is an essential part of the program so that the interface can load
        # while background tasks are being performed
        Thread(target=self.read_classes).start()

        # These enable keyboard shortcuts
        self.root.bind("<Control-s>", self.save_scores)
        self.root.bind("<Control-a>", self.add_class)
        self.root.bind("<Control-q>", lambda x: self.root.destroy())

        Label(self.root, text="Welcome to the Grade Averager" + self.title_extension).grid(row=0, columnspan=6, padx=10, pady=20)

        Label(self.root, text="Category").grid(row=1, column=0, padx=10, sticky=E)
        score_label = Label(self.root, text="Score(s)")
        score_label.grid(row=1, column=1)
        createToolTip(score_label, "Separate with commas. Eg.\n90, 83, 98, 75")
        weight_label = Label(self.root, text="Weight (%)")
        weight_label.grid(row=1, column=2)
        createToolTip(weight_label, "How much weight does the category have?\nHint: They should add up to 100")
        if self.show_states[0]:
            self.drop_label = Label(self.root, text="Drop lowest grade?")
            self.drop_label.grid(row=1, column=3, padx=4)
            createToolTip(self.drop_label, "Discards the lowest score from Score(s)")
        if self.show_states[1]:
            self.extra_label = Label(self.root, text="Extra credit?")
            self.extra_label.grid(row=1, column=4, padx=4)
            createToolTip(self.extra_label, "Check if score is extra credit")

        # These lines of code set up the input points for the user. self.active_boxes
        # (initially set to 6) determines how many rows (or categories) to create
        temp_box = [(self.create_entries(row+2, 1), self.create_entries(row+2, 2)) for row in range(self.active_boxes)]
        self.scores, self.weights = zip(*temp_box)

        temp_check = [(self.create_checks(row + 2, 3), self.create_checks(row+2, 4)) for row in range(self.active_boxes)]
        self.drop_low, self.extra_credit = zip(*temp_check)

        Label(self.root, text="    ").grid(row=1, column=5)

        self.fill_text()

        # The bottom frame is useful when having a large grid and items that need to be centered
        self.bottom_frame = Frame(self.root)
        self.bottom_frame.grid(row=self.active_boxes+2, columnspan=10)

        self.reset = Button(self.bottom_frame, text="Reset", command=lambda: self.restart(True))
        self.reset.grid(row=0, column=0, padx=10, pady=10)
        createToolTip(self.reset, "This closes out of the current class\nand clears all boxes without saving")

        self.submit = Button(self.bottom_frame, text="Submit", command=lambda: Thread(target=self.submit_form).start())
        self.submit.grid(row=0, column=1, padx=10, pady=10)

        self.status = Label(self.bottom_frame, text=self.status_fill)
        self.status.grid(padx=10, pady=10, row=1, columnspan=5)

        self.question = Label(self.root, text="Hover for help")
        self.question.grid(padx=3, pady=3, sticky=NE, row=0, column=self.farthest_left)
        createToolTip(self.question, "Enter your grades in Score(s)\nAnd what percent of the class it\ncounts for in Weight (%)\nAnd then hit Submit!\nFor more help, go to Help > Help")

        self.menu = Menu(self.root)
        self.root.config(menu=self.menu)

        # This code is responsible for the menu at the top. Due to the non-repeatable nature of a menu,
        # it's a bit of code, but is efficient where possible
        self.file_menu = Menu(self.menu, tearoff=False)
        self.menu.add_cascade(label='File', menu=self.file_menu)
        self.open_syl_menu = Menu(self.file_menu, tearoff=False)
        self.file_menu.add_cascade(label='Open Syllabus', menu=self.open_syl_menu)
        self.save_syl_menu = Menu(self.file_menu, tearoff=False)
        self.file_menu.add_cascade(label='Save Syllabus', menu=self.save_syl_menu)
        self.scores_load_menu = Menu(self.file_menu, tearoff=False)
        self.file_menu.add_separator()
        self.file_menu.add_cascade(label='Load Class', menu=self.scores_load_menu)

        self.save_command = self.file_menu.add_command(label='Save Scores', accelerator="Ctr+S", state=self.save_state, command=self.save_scores)
        self.file_menu.add_separator()

        self.file_menu.add_command(label='Quit', accelerator="Ctr+Q", command=self.root.destroy)

        self.customize_menu = Menu(self.menu, tearoff=False)
        self.menu.add_cascade(label='Customize', menu=self.customize_menu)
        self.naming = Menu(self.customize_menu, tearoff=False)
        self.customize_menu.add_cascade(label='Rename Class', menu=self.naming)

        self.customize_menu.add_command(label="Add Class", accelerator="Ctrl+A", command=self.add_class)
        self.delete_menu = Menu(self.customize_menu, tearoff=False)
        self.customize_menu.add_cascade(label="Delete Class", menu=self.delete_menu)

        self.customize_menu.add_separator()
        self.show_menu = Menu(self.customize_menu, tearoff=False)
        self.customize_menu.add_cascade(label="Show Checkboxes", menu=self.show_menu)
        self.show_menu.add_checkbutton(label="Show Drop Low", onvalue=1, offvalue=0, variable=self.show_drop, command=lambda: self.toggle_boxes(0))
        self.show_menu.add_checkbutton(label="Show Extra Credit", onvalue=1, offvalue=0, variable=self.show_extra_credit, command=lambda: self.toggle_boxes(1))

        # This loop adds the classes to the menu to be renamed, deleted, syllabus loaded, etc.
        for item in self.class_info:
            self.naming.add_command(label=item[0], command=lambda label=item[0]: self.rename_class(label))
            self.open_syl_menu.add_command(label=item[0], command=lambda label=item[0]: self.open_syl(label))
            self.save_syl_menu.add_command(label=item[0], command=lambda label=item[0]: self.save_syl(label))
            self.scores_load_menu.add_command(label=item[0], command=lambda label=item[0]: self.load_class(label))
            self.delete_menu.add_command(label=item[0], command=lambda row=item: self.delete_class(row))

        self.help_menu = Menu(self.menu, tearoff=False)
        self.menu.add_cascade(label='Help', menu=self.help_menu)
        self.help_menu.add_command(label='Help', command=self.help)
        self.help_menu.add_command(label='About', command=self.about)

        self.root.mainloop()

    # This function is called initially when the application is started and subsequently when the classes are
    # changed. The purpose is to read the classes into self.class_info so that it can properly display
    # the number of classes and their names and categories
    def read_classes(self):
        try:
            reading = open("Classes.csv")
            reader = csv.reader(reading)
            self.class_info = list(reader)
            reading.close()
        except FileNotFoundError:
            reading = open("Classes.csv", 'w', newline='')
            reading.close()
            messagebox.showinfo("Congrats!", "Hello! And thank you for using the\nworld's best averager")

    # This function fills the entries in the main root. self.class_index represents the index of the active
    # class with respect to the .csv file, so it is set to -1 when no class is active. Thus, this
    # function fills the entries with 0s.
    def fill_text(self):
        if self.class_index == -1:
            for row in range(self.active_boxes):
                label = Label(self.root, text="")
                label.grid(row=row+2, column=0, padx=10, sticky=E)
        else:
            for row in range(self.active_boxes):
                label = Label(self.root, text=self.class_info[self.class_index][row*5+1])
                label.grid(row=row + 2, column=0, padx=10, sticky=E)

    # This function creates an entry in the proper place and returns the object to be stored in an
    # array for easy looping capabilities
    def create_entries(self, row, col):
        entry = Entry(self.root)
        if self.class_index == -1:
            entry.insert(END, '0')
        else:
            entry.insert(END, self.class_info[self.class_index][5*(row-2)+col+1])
        entry.grid(pady=10, column=col, row=row)
        entry.bind("<Return>", lambda _: Thread(target=self.submit_form).start())
        return entry

    # This function is similar to the previous only for checkboxes, and it checks the box if the user
    # had previously saved the box checked.
    def create_checks(self, row, col):

        var = IntVar()
        check_box = Checkbutton(self.root, variable=var)
        if self.show_states[col - 3]:
            check_box.grid(padx=10, pady=10, column=col, row=row)
        if self.class_index != -1 and self.class_info[self.class_index][5 * (row - 2) + col + 1] == "1":
            check_box.select()
        return var, check_box

    # This method controls displaying the checkboxes on the right
    def toggle_boxes(self, box):
        self.show_states[box] = not self.show_states[box]
        states_update = open(self.show_file_name, 'wb')
        pickle.dump(self.show_states, states_update, pickle.HIGHEST_PROTOCOL)
        states_update.close()
        # These functions will seem backwards because I switch the value of self.show_states[box] before this line
        if not self.show_states[box]:  # If the box is currently being shown
            if not box:  # If the Drop Lowest Grade boxes are currently being shown
                self.drop_label.grid_forget()
                for child in self.drop_low:
                    child[1].grid_forget()
                if not self.show_states[1]:
                    self.farthest_left = 2
            else:  # If the Extra Credit Boxes are currently being shown
                self.extra_label.grid_forget()
                for child in self.extra_credit:
                    child[1].grid_forget()
                if self.show_states[0]:
                    self.farthest_left = 3
                else:
                    self.farthest_left = 2

        else:  # If the boxes are not currently being shown
            if not box:  # If the Drop Lowest Grade Boxes are not currently being shown
                self.drop_label = Label(self.root, text="Drop lowest grade?")
                self.drop_label.grid(row=1, column=3)
                self.drop_low = [self.create_checks(row + 2, 3) for row in range(self.active_boxes)]
                if not self.show_states[1]:
                    self.farthest_left = 3
            else:  # If the Extra Credit boxes are not currently being shown
                self.extra_label = Label(self.root, text="Extra credit?")
                self.extra_label.grid(row=1, column=4)
                self.extra_credit = [self.create_checks(row + 2, 4) for row in range(self.active_boxes)]
                self.farthest_left = 4

        self.question.grid(column=self.farthest_left)

    # Here we loop search self.class_info to find the class the user chose to open, and then when found,
    # set self.active_boxes to the number of categories previously saved and self.class_index to the row
    # that the class was found on
    def load_class(self, class_name):
        self.active_boxes = 6
        for i in range(len(self.class_info)):
            if self.class_info[i][0] == class_name:
                self.title_extension = ' - ' + class_name
                self.class_index = i
                break
        try:
            self.active_boxes = int((len(self.class_info[self.class_index])-1)/5)
        except NameError:
            self.status.config(text="Class not found\nDid you corrupt the file?")

        self.status_fill = "Class loaded!"
        self.save_state = "normal"
        self.restart(False)

    # This goes through all of the entries, takes the grades, parses the strings, averages them,
    # drops the lowest grade if necessary, weights the categories, and prints the result to the
    # screen. This is the only function that this program could not live without.
    def submit_form(self):
        bad = 0
        grades = []
        weights = []
        weighted_grades = []
        weight_sum = 0

        for box in range(self.active_boxes):
            drop = self.drop_low[box][0].get() if self.show_states[0] else 0

            grades.append(self.parse_grades(self.scores[box].get(), drop))
            weights.append(self.parse_grades(self.weights[box].get(), False))
            weighted_grades.append(grades[box] * weights[box])
            if not (self.show_states[1] and self.extra_credit[box][0].get()):
                weight_sum += weights[box]
            self.scores[box].config(bg='white')
            self.weights[box].config(bg='white')
            if isinstance(grades[box], bool):
                self.scores[box].config(bg='red')
                bad += 1
            if isinstance(weights[box], bool):
                self.weights[box].config(bg='red')
                bad += 1

        if bad:
            self.status.config(text="You had " + str(bad) + " invalid input(s)")
            return

        if not weight_sum:
            self.status.config(text="Error! All your weights were 0!")
            return

        final_grade = sum(weighted_grades) / weight_sum

        self.status.config(text="Your final grade is %.2f" % final_grade)

    # This function goes through self.root and destroys all of the children and then restores them.
    # It is usually called to update the menu or number of entries
    def restart(self, wipe):
        if wipe:
            self.save_state = "disabled"
            self.title_extension = ''
            self.status_fill = 'Window reset'
            self.class_index = -1
            self.active_boxes = 6
        for child in self.root.winfo_children():
            child.destroy()

        self.__init__()

    # This function goes through the user input and saves it to a class just as it is and then writes
    # the numbers and values to a .csv file, which I found to be the easiest, most concise way to store
    # this type of data, the amount of which varies so much
    def save_scores(self, *args):
        if self.save_state == "disabled":
            self.status.config(text="Sorry! No class is loaded!")
            return
        changed_line = [self.class_info[self.class_index][0]]

        for item in range(self.active_boxes):
            changed_line.extend((self.class_info[self.class_index][item*5+1], self.scores[item].get(), self.weights[item].get(), self.drop_low[item][0].get(), self.extra_credit[item][0].get()))

        self.class_info[self.class_index] = changed_line

        writing = open("Classes.csv", 'w', newline='')
        writer = csv.writer(writing)
        for row in self.class_info:
            writer.writerow(row)
        writing.close()
        self.status.config(text="Saved!")

    # These lines of code are responsible for deleting a class the user is done with
    # First they are prompted to make sure, then if the class they are deleting is the active class, then
    # the application needs to be reset to default values, else it can simply update Classes.csv and restart
    # the application to update the menu
    def delete_class(self, row):
        if not messagebox.askokcancel("Careful!", "You are trying to delete " + row[0] +
                                      "\nAre you sure you want to continue?"):
            return

        try:
            os.remove(row[0] + ".pdf")
        except FileNotFoundError:
            pass

        if self.class_index != -1 and self.class_info[self.class_index] == row:
            self.active_boxes = 6
            self.class_index = -1
            self.save_state = "disabled"
        self.class_info.remove(row)
        writing = open("Classes.csv", 'w', newline='')
        writer = csv.writer(writing)
        for row in self.class_info:
            writer.writerow(row)
        writing.close()

        self.restart(False)

    # This function opens another GUI in a separate window and allows the user to create another class
    # This function required the most error checking, which includes not having 2 classes with the same
    # name, not naming a class nothing, having too many categories, having too many classes, and having
    # no categories.
    def add_class(self, *args):

        if len(self.class_info) >= 20:
            self.status.config(text="Do you really need that many classes?")
            return

        if self.child_window_open:
            self.status.config(text="You already have one window open!")
            return

        self.child_window_open = True

        self.categories = 0
        self.line = []

        def submit_info(*args):
            cat = entry.get().title()
            entry.delete(0, END)
            title.config(text="What do you want to call your next category?")

            if cat == "Quit":
                if self.categories <= 1:
                    title.config(text="Oops! You need at least one category!")
                    return
                writing = open("Classes.csv", 'a', newline='')
                writer = csv.writer(writing)
                writer.writerow(self.line)
                writing.close()
                self.close(root)
                self.read_classes()
                self.restart(False)
                return
            if not cat:
                title.config(text="You can't name it nothing!")
                return
            if len(cat) > 12:
                cat = cat[:12] + "..."
            if self.categories == 0:
                for item in self.class_info:
                    if cat == item[0]:
                        title.config(text="You already have a class with that name!")
                        return
                self.line.append(cat)
            else:
                self.line.extend((cat, 0, 0, 0, 0))

            if self.categories == 0:
                title.config(text="What do you want to call your first catergory?")
            elif self.categories == 1:
                instructions.config(text='Type "Quit" when done')
            self.categories += 1
            if self.categories >= 15:
                entry.insert(END, "Quit")
                submit_info(self.line)
            return self.line

        root = Toplevel()
        root.title("Add a Class")
        img = PhotoImage(file='favicon.gif')
        root.tk.call('wm', 'iconphoto', root._w, img)

        title = Label(root, text="What is the name of your class to be called?")
        title.pack(padx=10, pady=10)

        instructions = Label(root)
        instructions.pack(padx=10, pady=5)

        entry = Entry(root)
        entry.bind("<Return>", submit_info)
        entry.pack(padx=10, pady=10)

        button = Button(root, text="Submit", command=submit_info)
        button.pack()

        root.protocol("WM_DELETE_WINDOW", lambda: self.close(root))
        root.mainloop()

    # This method is quite similar to the first, only minus the categories. However, it still needs
    # to do much of the same error handling to rename a class
    def rename_class(self, to_be_renamed):
        if self.child_window_open:
            self.status.config(text="You already have one window open!")
            return

        self.child_window_open = True

        for item in range(len(self.class_info)):
            if self.class_info[item][0] == to_be_renamed:
                temp_index = item
                break

        def submit_name(*args):
            name = entry.get().title()
            if len(name) > 12:
                name = name[:12] + "..."

            if not name:
                title.config(text="You can't name it nothing!")
                return

            for item in self.class_info:
                if item[0] == name:
                    title.config(text="You already have a class with that name!")
                    return

            self.class_info[temp_index][0] = name

            writing = open("Classes.csv", 'w', newline='')
            writer = csv.writer(writing)
            for line in self.class_info:
                writer.writerow(line)
            writing.close()

            try:
                for syl in os.listdir():
                    if syl[:syl.rfind('.')] == to_be_renamed:
                        ending = syl[syl.rfind('.'):]
                        break
                else:
                    raise FileNotFoundError
                os.rename(to_be_renamed+ending, name+ending)
            except FileNotFoundError:
                pass

            self.close(root)
            self.restart(False)

        root = Toplevel()
        root.title("Rename a Class")
        img = PhotoImage(file='favicon.gif')
        root.tk.call('wm', 'iconphoto', root._w, img)

        title = Label(root, text="What do you want your class to be renamed to?")
        title.pack(padx=10, pady=10)

        entry = Entry(root)
        entry.bind("<Return>", submit_name)
        entry.pack(padx=10, pady=10)

        Button(root, text="Submit", command=submit_name).pack(padx=10, pady=10)

        root.protocol("WM_DELETE_WINDOW", lambda: self.close(root))
        root.mainloop()

    # This method comes handy because when configuring the root protocol to set self.child_window_open to false
    # when closing out of child windows, that way the program knows that there is not a child window open
    def close(self, root):
        self.child_window_open = False
        root.destroy()

    # This function is called from the submiting method and it used to check for valid input and parse strings
    # to retrieve numbers separated by commas
    def parse_grades(self, grades, drop):
        if not grades:
            return False
        if grades[0] == '#':
            return 0
        scores = grades.split(',')

        try:
            for num in range(len(scores)):
                scores[num] = float(scores[num])

        except ValueError:
            self.status.config(text="Invalid input!")
            return False

        if drop and len(scores) > 1:
            scores.sort()
            scores = scores[1:]

        avg = sum(scores) / len(scores)

        return avg

    # This function is responsible for saving the class syllabus on the computer.
    # It just asks the user where the syllabus is and then copies it to the cwd
    def save_syl(self, class_name):
        good_endings = ['.pdf', '.doc', '.docx', '.rtf', '.jpg', '.gif']
        syl_name = askopenfilename(filetypes=(("All files", "*.*"),
                                              ("PDF files", ("*.pdf", "*.PDF")),
                                              ("Word Documents", ("*.doc", "*.docx")),))

        if not syl_name:
            return

        ending = syl_name[syl_name.rfind('.'):]
        if ending.lower() not in good_endings:
            self.status.config(text="That's not a valid format, sorry")

        for file in os.listdir():
            if file.startswith(class_name):
                os.remove(file)

        shutil.copyfile(syl_name, class_name + ending)
        self.status.config(text="Syllabus Saved!")

    # This method opens the syllabus for a given class
    def open_syl(self, class_name):
        try:
            for syl in os.listdir():
                place = syl.rfind('.')
                if syl[:place] == class_name:
                    file_name = syl
                    break
            else:
                raise FileNotFoundError
            if sys.platform == "win32":
                os.startfile(file_name)
                self.status.config(text="Syllabus Opening...")
            else:
                opener = "open" if sys.platform == "darwin" else "xdg-open"
                subprocess.call([opener, file_name])
                self.status.config(text="Syllabus Opening...")
        except FileNotFoundError:
            self.status.config(text="No syllabus for this class yet!")

    # This is the code to open the help window that explains to the user how to use the program.
    # It is designed to be interactive to give less information at a time
    def help(self):
        if self.child_window_open:
            self.status.config(text="You already have one window open!")
            return
        self.child_window_open = True

        def config_text(*args):
            if var.get() == 'General':
                label.config(text=gen_text, image='', relief=FLAT)
                root.minsize(700, 200)
                root.maxsize(700, 200)
            if var.get() == 'Renaming Classes':
                label.config(text=rnme_text, image='', relief=FLAT)
                root.minsize(620, 200)
                root.maxsize(620, 200)
            if var.get() == 'Opening Your Syllabus':
                label.config(text=syl_text, image='', relief=FLAT)
                root.minsize(700, 200)
                root.maxsize(700, 200)
            if var.get() == 'Opening Saved Grades':
                label.config(text=grd_text, image='', relief=FLAT)
                root.minsize(700, 200)
                root.maxsize(700, 200)
            # elif var.get() == 'Sample Image':
            #     label.config(text='', image=self.photo, relief=RAISED)
            #     root.minsize(900, 500)
            #     root.maxsize(900, 500)

        root = Toplevel()
        root.minsize(700, 200)
        root.maxsize(700, 200)
        root.title("Help")
        img = PhotoImage(file='favicon.gif')
        root.tk.call('wm', 'iconphoto', root._w, img)

        var = StringVar(root)
        # initial value
        var.set('General')
        choices = ['General', 'Renaming Classes', 'Opening Your Syllabus', 'Opening Saved Grades', 'Sample Image']
        option = OptionMenu(root, var, *choices, command=config_text)

        option.grid(pady=20, padx=10, sticky=S)

        root.title('Help')

        gen_text = "Welcome to the Grade Averager!\n\n" \
                   "In the grade percentage, enter as many grades as you want in the\n" \
                   "same box separated by commas and they will be averaged. Then put the\n" \
                   "percentage of weight they have and enter it. i.e. Feel free to leave any boxes\n" \
                   "Please feel free to give suggestions and comments! Also, you can now\n" \
                   "start a line with a \"#\" to skip over that box, in case you want to\n" \
                   "leave values for a later time"

        rnme_text = "In order to rename a class go to Customize > Rename Class and\n" \
                    "then pick the class you want to rename. Type what you want to\n" \
                    "rename it to and press Enter. It really is that easy"

        syl_text = "It is now easier than ever to keep track of syllabi! Just go to\n" \
                   "File > Save Syllabus and click on the appropriate class, then find\n" \
                   "the syllabus. When you select it, the program will save it. Then\n" \
                   "next time you want to open your syllabus just go to File > Open\n" \
                   "and it will open in your default .pdf reader"

        grd_text = "In order to save a class you must first load a class by going\n" \
                   "to File > Load Class, and loading a created class. Then when you\n" \
                   "go to File > Save Class it will save all the data and options for\n" \
                   "the class"

        label = Label(root, text=gen_text)
        label.grid(row=0, column=1, rowspan=2, padx=30, pady=30, sticky=N)

        root.protocol("WM_DELETE_WINDOW", lambda: self.close(root))
        root.mainloop()

    # This is the code for the "about" window, like the credits at the end of the movie
    def about(self):
        if self.child_window_open:
            self.status.config(text="You already have one window open!")
            return
        self.child_window_open = True

        root = Toplevel()
        root.title('About this program')
        img = PhotoImage(file='favicon.gif')
        root.tk.call('wm', 'iconphoto', root._w, img)

        label = Label(root, text="This is a privately developed Python GUI Application\n"
                                  "developed soley by Matthew Niemiec. Feel free to use\n"
                                  "it as needed, just don't do anything stupid like showing\n"
                                  "it to your friends and saying it's yours. That's just\n"
                                  "rude. Special thanks to Adam Smith for critiques and\n"
                                  "general guidance. You're the coolest, Adam!")
        label.pack(side=TOP, padx=30, pady=30)

        root.protocol("WM_DELETE_WINDOW", lambda: self.close(root))
        root.mainloop()


# Checking to make sure that the script is being executed
if __name__ in "__main__:":
    Gui()
