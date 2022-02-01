from pathlib import Path

potential_solutions_file = (Path(__file__).parent / "potential-solutions.txt").resolve(strict=True)
potential_solutions = potential_solutions_file.read_text().splitlines()

permitted_guesses_file = (Path(__file__).parent / "permitted-guesses.txt").resolve(strict=True)
permitted_guesses = permitted_guesses_file.read_text().splitlines()
permitted_guesses.extend(potential_solutions)
