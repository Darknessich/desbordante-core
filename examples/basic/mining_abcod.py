import desbordante

TABLE = 'examples/datasets/abcod/reprise_records.csv'

print('''The abcOD miner checks every column X against every numeric column Y:
rows are sorted by X and split into series (see verifying_abcod.py).
X -> Y is reported if the bands of series with at least min_series_size
rows cover at least min_coverage of the known values of Y.
''')

miner = desbordante.abcod.algorithms.Default()
miner.load_data(table=(TABLE, ',', True))
miner.execute(delta=1, epsilon=1, direction='bidirectional', min_coverage=0.9,
              min_series_size=5)
names = ['catalog_number', 'title', 'country', 'year']
for od in miner.get_band_ods():
    print(f'{names[od.lhs]} -> {names[od.rhs]}: {len(od.series)} series, '
          f'coverage {od.coverage:.2f}')
    for series in od.series:
        print(f'  {len(series.rows)} rows, year {series.direction}, '
              f'{len(series.outliers)} outliers')
