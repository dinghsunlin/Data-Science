# Data Science HW3

## Run sample code
I have already set the preferred hyperparameters in the program.
Details will be described in the next section.

The program will automatically run on gpu 0.
If you want to choose the specific gpu, you can just type in the number.

For example, set the program to run on gpu 1:
```python
python3 train.py --gpu 1
```

Or you don't want to use gpu:
```python
python3 train.py --gpu -1
```

## Default hyperparameters
```python
epochs: 1500
lr: 0.001
wd: 1e-5
temp: 1
hid_dim: 128
out_dim: 128
dfr1: 0.4
dfr2: 0.3
der1: 0.2
der2: 0.1
```