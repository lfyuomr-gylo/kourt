import argparse


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--runner', required=True, help='Path to runner executable')
    parser.add_argument('-c', '--config', required=True, help='Path to test suite configuration file')
    parser.add_argument('-s', '--solution', required=True, help='Path to solution file')
    return parser.parse_args()
