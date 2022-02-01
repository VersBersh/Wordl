import subprocess
from pathlib import Path

from .data import permitted_guesses_file, potential_solutions_file, permitted_guesses
from .game import Game


def optimise(init_guess: str,
             max_tree_depth: int = 7,
             top_n: int = 6,
             max_n: int = 50) -> None:
    """ find the best game-tree for the given starting word

      :init_guess: the first guess to use as the root of the tree
      :max_tree_depth: a restriction on the maximum depth of the tree
           (speeds up tree search, should not be less than 5 or no
           solution will exist)
      :top_n: to optimise each node of the tree we take all permitted
           guesses, sort them by a heuristic, and check the top_n
      :max_n: if the heuristic cannot identify the top_n because there
           are multiple words with the same heuristic then we must
           test them all. max_n places a hard limit on the number of
           branches we can check
    """
    root = Path(__file__).parent.parent
    optimiser = root / "bin/wordl.exe"
    if not optimiser.exists():
        raise Exception("Missing Binary. ./bin/wordl.exe does not exist. "
                        " You must build the c++ first.")

    if init_guess not in permitted_guesses:
        raise Exception(f"Initial guess not in permitted guesses: {init_guess}")

    out_file = root / f"solutions/{init_guess}.txt"

    result = subprocess.run([optimiser.as_posix(),
                            init_guess,
                            potential_solutions_file.as_posix(),
                            permitted_guesses_file.as_posix(),
                            str(top_n),
                            str(max_n),
                            str(max_tree_depth),
                            out_file.as_posix()],
                            stdout=subprocess.PIPE)

    result.check_returncode()

    player = Game(init_guess)
    print(player.print_stats())
    print(result.stdout.decode("utf-8").splitlines()[1])
