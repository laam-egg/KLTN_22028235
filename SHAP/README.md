# xAI

To explain the models' decisions.

## Setup

```powershell
cd $PROJECT_ROOT
pip install .
pip install -r myscripts/requirements.txt

cd $PROJECT_ROOT/my_scripts/xAI
pip install -r ./requirements.txt
```

## Run

```powershell
python app.py
```

Then go to <http://localhost:8000>.

Default master password: `changeme`.
To change it, set the environment variable
`XAI_MASTER_PASSWORD` to the desired
password before running the above
command.

## Implementation Details

### Feature Extractor, C++ Implementation

An optimized C++ implementation of the EMBER2024
feature extractor. We currently use it in place
of the original feature extractor (which is
written in Python). Refer to its README for
strengths and tradeoffs.

The `.dll` and `.so` files are the Release
binaries of the new feature extractor, and is
included in Git commits here for convenience.
The `efe_cpp.py` contains the wrapper code
to actually use it.

### Research

The file `xAI.ipynb` is just a draft for research.
It is not for production. You could take a look
at it out of curiosity.
