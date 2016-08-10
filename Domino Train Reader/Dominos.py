from random import randint
import cv2
import numpy as np

spinner = (int(input("What is the spinner?")),)

all_trains = ()


def mark_circles(circles, lineX):
    global domino
    test = [0, 0]
    if circles is not None:
        for (x, y, r) in circles:
            if x > lineX:
                test[0] += 1
            else:
                test[1] += 1

    for index in range(2):
        domino[index]=test[index] if test[index] > domino[index] else domino[index]


def marking(checking):
    cap = cv2.VideoCapture(0)
    iteration = 0
    lineX = 0
    while True:
        if checking:
            iteration += 1
            if iteration > 15:
                break
        _, frame = cap.read(cv2.WINDOW_NORMAL)
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        template = cv2.imread('black_line.png', 0)
        w, h = template.shape[::-1]

        res = cv2.matchTemplate(gray, template, cv2.TM_CCOEFF_NORMED)
        threshold = 0.55
        loc = np.where(res >= threshold)

        for pt in zip(*loc[::-1]):
            cv2.rectangle(frame, pt, (pt[0]+w, pt[1]+h), (255, 255, 255), 2)
            lineX = pt[0]
            break

        circles = cv2.HoughCircles(gray, cv2.HOUGH_GRADIENT, 0.8, 20, param1=65, param2=21, minRadius=10, maxRadius=35)

        # ensure at least some circles were found
        if circles is not None:
            # convert the (x, y) coordinates and radius of the circles to integers
            circles = np.round(circles[0, :]).astype("int")

            # loop over the (x, y) coordinates and radius of the circles
            for (x, y, r) in circles:

                # draw the circle in the output image, then draw a rectangle
                # corresponding to the center of the circle
                cv2.circle(frame, (x, y), r, (0, 255, 0), 4)
                cv2.rectangle(frame, (x - 5, y - 5), (x + 5, y + 5), (0, 128, 255), -1)

        if checking:
            mark_circles(circles, lineX)

        # show the output image
        cv2.imshow("output", frame)

        cv2.imshow('frame', frame)

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

    cap.release()
    cv2.destroyAllWindows()


def make_dominos():
    test_train = ()
    for _ in range(12):
        test_add = ((randint(0, 12), randint(0, 12)),)
        test_train += test_add

    print(test_train)
    return test_train


def find_longest_path(decided, test):
    global all_trains
    try:
        compare = decided[-1][-1]
    except TypeError:
        compare = decided[-1]

    if not test:
        all_trains += (decided,)

    for item in test:
        found = False
        new_decided = decided
        new_test = item
        if item[0] == compare:
            found = True
        elif item[1] == compare:
            found = True
            new_test = item[::-1]
        if found:
            new_decided += (new_test,)
            new_test = tuple(tup for tup in test if tup != item)
            find_longest_path(new_decided, new_test)
        else:
            if new_decided != spinner:
                all_trains += (new_decided,)


def main():
    global domino

    domino_set = ()

    for _ in range(2):
        domino = [0, 0]
        print("Adjust domino on screen and press 'q' to capture domino")
        marking(False)
        marking(True)
        domino_set += ((domino[0], domino[1]),)
        print(domino_set)

    #domino_set = make_dominos()
    print(domino_set)
    # print(spinner[-1][-1])
    find_longest_path(spinner, domino_set)

    longest_train = ()
    max_length = 0

    for item in all_trains:
        if len(item) >= max_length and item not in longest_train:
            longest_train += (item,)
            max_length = len(item)

    if max_length:
        print("The longest train(s) for you is ")
        for item in longest_train:
            if len(item) == max_length:
                print(item[1:])
        print("with a length of " + str(max_length-1))
    else:
        print("No train for you. Sorry :(")

if __name__ in '__main__':
    main()
