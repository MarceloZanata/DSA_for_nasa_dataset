import pandas as pd

csv_path = 'dataset_projeto1/asteroid_dataset_20251019.csv'

df = pd.read_csv(csv_path, dtype_backend="numpy_nullable")

print(df.dtypes)

