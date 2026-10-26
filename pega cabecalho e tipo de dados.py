import pandas as pd

csv_path = 'dados/asteroids.csv'

df = pd.read_csv(csv_path)

print(df.dtypes)

