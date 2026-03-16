from typing import Any, Callable, Literal, override
import numpy as np
import shap

from feature_locations import FeatureLocation, FeatureLocationGroup, FeatureLocationAnalyzer

class AbstractFeatureExtractor:
    def extract_from_bytes(self, bytez: bytes) -> np.ndarray:
        raise NotImplementedError
    
    def extract_from_file(self, file_path: str) -> np.ndarray:
        raise NotImplementedError
    
    def get_features(self) -> list[FeatureLocation | FeatureLocationGroup]:
        raise NotImplementedError
    
class AbstractModel:
    def predict_sample(self, feature_vector: np.ndarray) -> float:
        raise NotImplementedError
    
    def get_raw_model_instance(self) -> Any:
        raise NotImplementedError

from dataclasses import dataclass, asdict
from typing import Type, TypeVar

T = TypeVar("T", bound="AbstractRecord")

@dataclass
class AbstractRecord:
    def to_dict(self) -> dict:
        return asdict(self)

    @classmethod
    def from_dict(cls: Type[T], d: dict) -> T:
        return cls(**d)

@dataclass
class Contribution(AbstractRecord):
    name: str
    contribution: float
    impact: str
    value: float | None = None

@dataclass
class ModelExplanation(AbstractRecord):
    type: Literal['logit'] | Literal['prob']
    baseline: float
    logit: float
    prob: float
    sorted_contributing_features: list[Contribution]
    sorted_contributing_feature_groups: list[Contribution]
    sorted_contributing_feature_categories: list[Contribution]

class AbstractModelExplainer:
    def __init__(
        self,
        fe: AbstractFeatureExtractor,
        model: AbstractModel,
    ):
        self.extractor = fe
        self.model = model

    def explain_from_bytes(self, bytez: bytes):
        feature_vector = self.extractor.extract_from_bytes(bytez)
        return self._explain_with_feature_vector(feature_vector)
    
    def explain_from_file(self, file_path: str):
        feature_vector = self.extractor.extract_from_file(file_path)
        return self._explain_with_feature_vector(feature_vector)

    def _explain_with_feature_vector(self, feature_vector: np.ndarray) -> ModelExplanation:
        raise NotImplementedError










def logit(y):
    # Ensure y is within the valid range (0, 1) to avoid math errors
    if not (0 < np.min(y) and np.max(y) < 1):
        raise ValueError("Input 'y' must be in the range (0, 1)")
        
    return np.log(y / (1 - y))

def sigmoid(x):
    """
    Computes the element-wise sigmoid of x.

    x: A single number, a NumPy array, a vector, or a matrix.
    Returns: The sigmoid value or array of values between 0 and 1.
    """
    return 1 / (1 + np.exp(-x))

def logit_to_odds_multiplier(delta_logit: float) -> float:
    return np.exp(delta_logit)

def format_odds_change(delta_logit: float) -> str:
    mult = np.exp(delta_logit)

    if mult >= 1:
        return f"↑ INCREASES malware odds by {mult:.1f}×"
    else:
        return f"↓ REDUCES malware odds by {1/mult:.1f}×"



class TreeModelExplainer(AbstractModelExplainer):
    def __init__(
        self,
        fe: AbstractFeatureExtractor,
        model: AbstractModel,
    ):
        super().__init__(fe, model)
        self.explainer = shap.TreeExplainer(
            self.model.get_raw_model_instance()
        )
    
    @override
    def _explain_with_feature_vector(self, feature_vector: np.ndarray) -> ModelExplanation:
        features = self.extractor.get_features()
        fa = FeatureLocationAnalyzer(
            features
        )
        model = self.model.get_raw_model_instance()

        a2d = np.array([feature_vector])
        shap_values = self.explainer.shap_values(a2d)
        baseline = float(self.explainer.expected_value) # type: ignore
        pred = float(model.predict(a2d)[0])
        logit = float(np.log(pred / (1 - pred)))
        
        # Sanity check BEGINS
        assert np.abs(sigmoid(logit) - pred) < 1e-4

        logit_reconstructed = baseline + shap_values.sum()
        prob_from_shap = sigmoid(logit_reconstructed)
        assert np.abs(pred - prob_from_shap) < 1e-4
        # Sanity check ENDS

        FEATURE_LIST = fa.feature_list
        shap_vals = list(shap_values[0])
        feature_contribs = list(zip(FEATURE_LIST, shap_vals))
        feature_contribs.sort(key=lambda x: abs(x[1]), reverse=True)
        sorted_contributing_features = [
            Contribution(
                name=feature.get_fqfn(),
                contribution=val,
                impact=format_odds_change(val),
                value=feature_vector[feature.location],
            )

            for feature, val in feature_contribs
        ]

        feature_groups_impacts_dict: dict[str, float] = {}
        for feature_group_name, val in [(feature.path[0].group_name, val) for (feature, val) in feature_contribs]:
            feature_groups_impacts_dict[feature_group_name] = (
                feature_groups_impacts_dict.get(feature_group_name, 0) + val
            )
        feature_groups_impacts = sorted(feature_groups_impacts_dict.items(), key=lambda x: abs(x[1]), reverse=True)
        sorted_contributing_feature_groups = [
            Contribution(
                name=feature_group_name,
                contribution=val,
                impact=format_odds_change(val),
            )

            for feature_group_name, val in feature_groups_impacts
        ]

        feature_categories_impacts_dict: dict[str, float] = {}
        for categories, val in [(feature.categories, val) for (feature, val) in feature_contribs]:
            if len(categories) > 0:
                for cat in categories:
                    cat_name = cat.category_name
                    feature_categories_impacts_dict[cat_name] = (
                        feature_categories_impacts_dict.get(cat_name, 0) + val
                    )
            else:
                cat_name = "others"
                feature_categories_impacts_dict[cat_name] = (
                    feature_categories_impacts_dict.get(cat_name, 0) + val
                )
        feature_categories_impacts = sorted(feature_categories_impacts_dict.items(), key=lambda x: abs(x[1]), reverse=True)
        sorted_contributing_feature_categories = [
            Contribution(
                name=feature_category_name,
                contribution=val,
                impact=format_odds_change(val),
            )

            for feature_category_name, val in feature_categories_impacts
        ]

        result = ModelExplanation(
            type='logit',
            baseline=baseline,
            logit=logit,
            prob=pred,
            sorted_contributing_features=sorted_contributing_features,
            sorted_contributing_feature_groups=sorted_contributing_feature_groups,
            sorted_contributing_feature_categories=sorted_contributing_feature_categories,
        )

        return result

# ==================================== #
from efe_cpp import FeatureExtractor

class EMBER2024CppFeatureExtractor(AbstractFeatureExtractor):
    def __init__(self):
        self.extractor = FeatureExtractor()
    
    @override
    def extract_from_bytes(self, bytez: bytes) -> np.ndarray:
        return np.array(
            self.extractor.extract_from_bytes(bytez)
        )
    
    @override
    def extract_from_file(self, file_path: str) -> np.ndarray:
        return np.array(
            self.extractor.extract_from_file(file_path)
        )
    
    @override
    def get_features(self) -> list[FeatureLocation | FeatureLocationGroup]:
        from feature_locations import features
        return features

from pathlib import Path
import lightgbm as lgb
MODELS_PATH = Path("../../models")
class EMBER2024LGBMModel(AbstractModel):
    def __init__(self):
        self.model = lgb.Booster(model_file=MODELS_PATH / "EMBER2024_all.model")
    
    @override
    def predict_sample(self, feature_vector: np.ndarray) -> float:
        return self.model.predict([feature_vector])[0] # type: ignore
    
    @override
    def get_raw_model_instance(self) -> Any:
        return self.model

explainer = TreeModelExplainer(
    fe=EMBER2024CppFeatureExtractor(),
    model=EMBER2024LGBMModel(),
)

def explain(file_path: str) -> str:
    explanation = explainer.explain_from_file(file_path)

    import json

    # with open("explanation.json", 'w') as f:
    #     json.dump(explanation.to_dict(), f)

    explanation_json = json.dumps(explanation.to_dict())

    def export_html(json_content: str) -> str:
        """
        Export benchmark results JSON to an HTML report.
        """
        # Load the HTML template using importlib.resources
        template_file_path = Path(".") / 'explanation.html.template'
        with open(template_file_path, 'r', encoding='utf-8') as f:
            template_content = f.read()
        
        # Replace the placeholder with JSON content
        html_output = template_content.replace(
            '[[EXPLANATION_DATA]]',
            json_content
        )
        
        return html_output
        # except json.JSONDecodeError as e:
        #     print(f"[ERROR] Invalid JSON in input file: {e}")
        # except FileNotFoundError as e:
        #     print(f"[ERROR] File not found: {e}")
        # except Exception as e:
        #     print(f"[ERROR] Failed to generate HTML report: {e}")

    return export_html(explanation_json)

# html_output = explain("Z:\\SHARED\\ProcessHollowing.exe")
# with open("explanation.html", 'w', encoding='utf-8') as f:
#     f.write(html_output)
# ==================================== #
