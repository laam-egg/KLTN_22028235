from ..data import load_dataset
from .training import train_pipeline
from .evaluation import evaluate

def all_in_one(BUILD: bool, dataset_path: str):
    print("Loading dataset")
    (
        train_samples,
        train_sample_to_caps,
        train_sample_to_behaviors,
        train_sample_to_tactics, # unused for now
        labels
    ) = load_dataset(
        dataset_path,
        split="train"
    )
    if BUILD:

        print("Training")
        emb_model, gmm, lr = train_pipeline(
            True,
            train_samples,
            train_sample_to_caps,
            train_sample_to_behaviors,
            labels
        )
    else:
        print("Training")
        emb_model, gmm, lr = train_pipeline(
            False,
            train_samples,
            train_sample_to_caps,
            train_sample_to_behaviors,
            labels
        )

    # (
    #     test_samples,
    #     test_sample_to_caps,
    #     test_sample_to_behaviors,
    #     test_sample_to_tactics, # unused for now
    #     labels_ # already loaded and is consistent
    # ) = load_dataset(
    #     dataset_path,
    #     split="test"
    # )

    #?
    test_samples = train_samples
    #?-
    print("Evalating...")
    metrics = evaluate(test_samples, emb_model, gmm, lr, labels) # ???

    print(metrics)
