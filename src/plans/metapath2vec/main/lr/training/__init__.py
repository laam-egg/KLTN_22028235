from sklearn.linear_model import LogisticRegression

def train_lr(VCP, y):
    lr = LogisticRegression(max_iter=1000)
    lr.fit(VCP, y)
    return lr
