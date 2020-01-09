## Setup Development Environment

1. Install Python 3.8 or higher
1. Create and activate virtualenv
   ```bash
   python -m venv .venv
   source .venv/bin/activate
   ```
1. Install dependencies
   ```bash
   pip install -r requirements.txt
   ```

## Run tests

To run Python tests, simply run 
```bash
python -m pytest
```

To execute runner unit-tests, see instructions in [runner build instructions](./runner/README.md)