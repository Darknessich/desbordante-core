import desbordante
import pandas as pd

TABLE = 'examples/datasets/abcod/reprise_records.csv'
CATALOG, YEAR = 0, 3


def describe(table, series):
    first = table.iloc[series.rows[0]]['catalog_number']
    last = table.iloc[series.rows[-1]]['catalog_number']
    print(f'  {first} .. {last}: {len(series.rows)} releases, year {series.direction}, '
          f'{series.lmb_size} of {series.non_null_count} in the band')
    for row in series.outliers:
        record = table.iloc[row]
        print(f'    outlier: {record["catalog_number"]} "{record["title"]}", '
              f'year {int(record["year"])}')


print('''Record labels assign catalog numbers in order, so the release year
mostly grows with the catalog number. A band order dependency (band OD)
"catalog_number ->_delta year" allows the year to go back by at most delta:
a number may be assigned before the release is out.

Real data break even a band OD in two ways:
- errors: a few years are wrong (approximation, abOD);
- several numberings: the dependency holds on series of rows
  (conditioning, bcOD).
abcOD combines both: rows sorted by X are split into series; every series
has a longest monotonic band (LMB), the other rows are outliers. The
series maximize the gain while no series has more than epsilon outliers
in a row. See Li et al., "ABC of Order Dependencies" (2020).
''')

table = pd.read_csv(TABLE, dtype={'year': 'Int64'})
verifier = desbordante.abcod_verification.algorithms.Default()
verifier.load_data(table=(TABLE, ',', True))

print('1) abOD: one band over the whole table, delta = 1, year ascending')
verifier.execute(lhs_indices=[CATALOG], rhs_indices=[YEAR], delta=1)
print(f'  error: {verifier.get_error():.3f} '
      f'({len(verifier.get_outliers())} of 21 known years are outliers)')
print(f'  holds with error 0.1: {verifier.holds(0.1)}')
print()

print('2) bcOD: the least number of segments where the band OD holds exactly')
verifier.execute(lhs_indices=[CATALOG], rhs_indices=[YEAR], delta=1,
                 direction='bidirectional')
for segment in verifier.get_segments():
    describe(table, segment)
print()

print('3) abcOD: series with at most epsilon = 1 outlier in a row, '
      'each series may go up or down')
verifier.execute(lhs_indices=[CATALOG], rhs_indices=[YEAR], delta=1, epsilon=1,
                 direction='bidirectional')
for series in verifier.get_series():
    describe(table, series)
print(f'  gain: {verifier.get_gain()}')
print()
print('The only outlier is "Mirror Ball": its year 2012 is far from its neighbours '
      '(the true year is 1995).\nThe missing year of "Ancient Heart" is not an outlier.')
