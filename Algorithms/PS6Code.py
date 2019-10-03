from random import random, randint
import numpy as np

COST_INDEL = 1
COST_SWAP = 10
COST_SUB = 10


def print_matrix(matrix):
    print(np.matrix(matrix))


# This is going to return the minimal cost of aligning 2 strings
# For parameters, string1 is the string on top and string2 is the string on the
# side. Doesn't matter for the algorithm, but it is useful for testing purposes
def align_strings(string1, string2):
    # This is a nested function that calculates the cost. It is helpful because
    # it keeps the bulk of the code clean and has access to all of this
    # function's local variables, seen as global variables in this context
    def cost(i, j):
        costs = []

        # The top left corner of the matrix
        if i == j == 0:
            return 0

        # Costs of indel operations
        if i > 0:
            costs.append(matrix[i-1][j] + COST_INDEL)
        if j > 0:
            costs.append(matrix[i][j-1] + COST_INDEL)

        # Cost of sub
        # Doesn't take into account the space at the beginning
        if i > 0 and j > 0:
            if string1[j-1] == string2[i-1]:
                costs.append(matrix[i-1][j-1])
            else:
                costs.append(matrix[i-1][j-1] + COST_SUB)

        # Cost of swap
        # Also doesn't take into account the space at the beginning
        if i > 1 and j > 1:
            swap = (string1[j-1] != string2[i-2] + string1[j-2] != string2[i-1])
            swap *= COST_SUB
            swap += matrix[i-2][j-2] + COST_SWAP
            costs.append(swap)
        # Return the minimum of all the available ops
        return min(costs)

    # Create an mxn matrix based on the lengths of the strings
    matrix = [[0]*(len(string1)+1) for _ in range((len(string2)+1))]

    for i in range(len(matrix)):
        for j in range(len(matrix[0])):
            matrix[i][j] = cost(i, j)
    return matrix


# Returns a vector the represents an optimal solution that converts x into y
def extract_alignment(matrix, string1, string2):
    high_bound = max((x for y in matrix for x in y)) + 1
    vector = []
    i = len(string2)
    j = len(string1)
    while i > 0 or j > 0:
        choices = []
        # Potential case of an indel
        if i > 0:
            choices.append(matrix[i-1][j])
        else:
            choices.append(high_bound)
        # Potential case of the other indel
        if j > 0:
            choices.append(matrix[i][j-1])
        else:
            choices.append(high_bound)
        # Potential case of a sub
        if i > 0 and j > 0:
            choices.append(matrix[i-1][j-1])
        else:
            choices.append(high_bound)
        # Case where there was potentially a swap
        if i > 1 and j > 1:
            diff = matrix[i][j] - matrix[i - 2][j - 2]
            subs = (string1[j-1] != string2[i - 2]) + (string1[j - 2] != string2[i-1])
            subs *= COST_SUB
            subs += COST_SWAP
            # Check if it's possible for a swap to have occurred
            if diff == subs:
                choices.append(matrix[i-2][j-2])
            else:
                choices.append(high_bound)
        else:
            choices.append(high_bound)
        # Find the minimum possible previous number
        small = min(choices)
        # Find the number of times that min occurs
        routes = choices.count(small)
        # Make an int decision, which is a number between 1 and the times min occurred
        decision = int(random()*routes) + 1
        temp = -1
        # Each time we encounter min we decrement decision
        # When decision is 0, we choose that instance of min/path
        while decision > 0:
            temp += 1
            if choices[temp] == small:
                decision -= 1

        # Case where decision is the insert a space
        if temp == 0:
            vector.append('INS')
            i -= 1
        # Case where decision is to delete the letter. Append deletion symbol
        elif temp == 1:
            vector.append('DEL')
            j -= 1
        # Case of swap. If a match, insert accordingly
        elif temp == 2:
            # Letters align - case of no-op
            if small == matrix[i][j]:
                vector.append('|')
            else:
                vector.append('SUB')
            i -= 1
            j -= 1
        # Case where decision is to swap the two letters
        elif temp == 3:
            vector.append('SWAP')
            vector.append('SWAP')
            j -= 2
            i -= 2
    # Switch the vector order since it was initially backwards
    vector.reverse()
    return vector


# The function that takes in the list of edits and the first string
# and returns the total number of instances where the two strings were the same
def common_substrings(string1, edits, length):
    i = 0
    count = 0
    current_streak = 0
    # Go thought the length of the list of edits
    while i < len(edits):
        if edits[i] == '|':
            current_streak += 1
            # When there are 10 or more noops in a row
            if current_streak == length:
                # Print the specific case where the match occurs with plagiarism
                output = string1[i-length+1:i+1]
                if isinstance(output, list):
                    print(output)
                count += 1
                current_streak = 0
        else:
            current_streak = 0
        i += 1
    return count


# This is code that we use to test our functions above. The results are
# What was given in class
s1 = "polynomial"
s2 = "exponential"
MIN_COMMON_LENGTH = 3
cost_matrix = align_strings(s1, s2)  # In-class exercise. The result should be 3
print_matrix(cost_matrix)
fix = extract_alignment(cost_matrix, s1, s2)
print(fix)
common_strings = common_substrings(s1, fix, 3)
print(common_strings)

# This is for part c. We renamed the files so that they weren't really long to type
reading = open('data_string_x.txt', 'r')
data_x = reading.read().split()
reading.close()
reading = open('data_string_y.txt', 'r')
data_y = reading.read().split()
reading.close()

text_matrix = align_strings(data_x, data_y)
text_fix = extract_alignment(text_matrix, data_x, data_y)
text_lengths = common_substrings(data_x, text_fix, 10)
print("Next:", text_lengths)

# read in the files, make them lowercase, split them up into words, and only take into
# account the first 2000 so that our algorithm doesn't take a really long time to run
reading = open('whitehouse.txt', 'r')
test1 = reading.read().lower().split()[:2000]
reading.close()
reading = open('fema.txt', 'r')
test2 = reading.read().lower().split()[:2000]
reading.close()

# Run the same tests again
text_matrix = align_strings(test1, test2)
text_fix = extract_alignment(text_matrix, test1, test2)
text_lengths = common_substrings(test1, text_fix, 10)
print("Next:", text_lengths)

# We read in the last files
reading = open('muchAdo_txt.txt', 'r')
ado = reading.read()
reading.close()
reading = open('muchAdo_freqs.txt', 'r')
ado_freq = reading.read()
reading.close()
monkey_string = ""
for line in ado_freq.split('\n'):
    line = line.split()
    if len(line) == 3:
        line[0] = "' '"
    monkey_string += line[0][1]*int(line[-1])
# monkey_string = ''.join(sample(monkey_string, len(monkey_string)))
monkey_length = len(monkey_string)
n = monkey_length
ado_lengths = 0
runs = 0
while not ado_lengths:
    print("Nope")
    test_string = ""
    for _ in range(n):
        test_string += monkey_string[randint(0, monkey_length-1)]
    # print(test_string)
    ado_matrix = align_strings(ado, test_string)
    ado_fix = extract_alignment(ado_matrix, ado, test_string)
    ado_lengths = common_substrings(ado, ado_fix, 7)
    runs += 1
print("Finally! n =", n*runs)
print("Number of matches:", ado_lengths)