from collections import Counter, defaultdict
from pathlib import Path
from typing import *

from .data import potential_solutions

Information = NewType("Information", Tuple[int, int, int, int, int])


class Node:
    def __init__(self, best_guess: str, max_depth: int,
                 children: Dict[Information, List["Node"]]):
        self.best_guess = best_guess
        self.max_depth = max_depth
        self.children = children

    def __str__(self):
        return f"<Node {self.best_guess} {self.max_child}>"

    def __repr__(self):
        return str(self)


class Game:
    help_text = """
        To play wordl using the wordl solver:
            1. visit powerlanguage.co.uk/wordl/
            2. call Game().play() in a python console
                - you can call, e.g., Game("reast") to play the
                  game using "reast" as the first guess. You need
                  to build the solution first using wordl.solve("reast")
            3. input the guess from the game
            4. input the information/clues returned by words as a sequence
              of numbers where Grey=0, Yellow=1, and Green=2
              So if you guess "salet" and "a" was green and "e" was yellow
              then you should input 02010 and press enter to get the next
              guess.
    """

    def __init__(self, first_guess: str = "salet"):
        self.first_guess = first_guess
        root = Path(__file__).parent.parent
        solution = root / f"solutions/{first_guess}.txt"
        if not solution.exists():
            raise Exception(f"Solution for initial guess {first_guess} does not"
                            f"exist. You must run wordl.solve('{first_guess}')")


        words = [line.split(", ") for line in solution.read_text().splitlines()]
        self.root = Game.words_to_tree(words)

    @staticmethod
    def compute_info(guess: str, target: str) -> Information:
        """ calculate the information from guesses guess when
            the actual word is target
        """
        result = [0] * 5

        guess_letters = Counter(guess)
        target_letters = Counter(target)
        common_letters = set(guess) & set(target)

        common_with_mult = {
            letter: min(guess_letters[letter], target_letters[letter])
            for letter in common_letters
        }

        for index, (g, t) in enumerate(zip(guess, target)):
            if g == t:
                result[index] = 2
                common_with_mult[g] -= 1

        for index, g in enumerate(guess):
            if common_with_mult.get(g, 0) != 0 and result[index] != 2:
                result[index] = 1
                common_with_mult[g] -= 1

        assert sum(common_with_mult.values()) == 0

        return tuple(result)

    @staticmethod
    def words_to_tree(words: List[List[str]]) -> Node:
        """ convert solution output back into a tree """
        guess = words[0][0]
        grouping = defaultdict(list)
        for w in words:
            assert w[0] == guess, (w[0], guess)
            info = Game.compute_info(guess, w[-1])
            if not isinstance(w, list):
                print(w)
                raise Exception
            if len(w) == 1:
                continue
            grouping[info].append(w[1:])

        children = {}
        for info,v in grouping.items():
            children[info] = Game.words_to_tree(v)

        if len(words) == 1:
            max_depth = 1
        else:
            max_depth = 1 + max(children.values(), key=lambda x: x.max_depth).max_depth

        return Node(guess, max_depth, children)

    def get_guesses(self, target: str) -> List[str]:
        """ return the guesses that the game would use to solve
            the given target
        """
        node = self.root
        guesses = [node.best_guess]
        while (node.best_guess != target):
            info = Game.compute_info(node.best_guess, target)
            node = node.children[info]
            guesses.append(node.best_guess)

        return guesses

    def play(self):
        """ Use the bot to beat Wordl """
        node = self.root
        guess_no = 1
        print(f"Guess {guess_no}: {node.best_guess}")
        while True:
            raw_info = input("Input information (or type 'help') >>\n")
            if raw_info.lower() == "help":
                print(Game.help_text)
                continue
            info = tuple(map(int, raw_info.strip()))
            if info not in node.children:
                print("Impossible. Double check your input.")
                answer = input("Enter A to abort. C to continue")
                if answer.lower() == "a":
                    return
            node = node.children[info]
            guess_no += 1
            if len(node.children) == 0:
                print(f"FINAL ANSWER: {node.best_guess}!")
                return
            else:
                print(f"Guess {guess_no}: {node.best_guess}")

    def print_stats(self) -> str:
        """ print some statistics about this first guess """
        results = []
        for target in potential_solutions:
            results.append(self.get_guesses(target))

        total_guesses = sum(len(g) for g in results)
        avg_depth = total_guesses / len(results)
        max_depth = len(max(results, key=len))

        return (f"Word: {self.first_guess}\n"
                f"Average Depth: {avg_depth}\n"
                f"Max Depth: {max_depth}\n"
                f"Total Guesses: {total_guesses}")
