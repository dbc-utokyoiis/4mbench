# 4mbench

4mbench is a tool for performance benchmark of manufacturing business datababase.

## Dataset generator

A dataset generator *4mdgen* is placed in the `4mdgen/` directory.

### Build the dataset generator

	$ make

### Generate a dataset of 10-day business operations (i.e., on 2022-04-01 to 2022-04-10) of a production line (#101) in the `/tmp` directory

	$ 4mdgen -l 101 -d 10 -o /tmp

### Generate a dataset of 12-day business operations (i.e., on 2022-04-01 to 2022-04-12) of three production lines (#101 to #103) in the `/tmp` directory (small-scale dataset)

	$ l=101; while [ $l -lt 104 ]; do 4mdgen -l $l -d 12 -o /tmp; l=`expr $l + 1`; done

### Generate a dataset of 40-day business operations (i.e., on 2022-04-01 to 2022-05-10) of 10 production lines (#101 to #103) in the `/tmp` directory (middle-scale dataset)

	$ l=101; while [ $l -lt 111 ]; do 4mdgen -l $l -d 40 -o /tmp; l=`expr $l + 1`; done

### Generate a dataset of 120-day business operations (i.e., on 2022-04-01 to 2022-07-29) of 30 production lines (#101 to #103) in the `/tmp` directory (large-scale dataset)

	$ l=101; while [ $l -lt 131 ]; do 4mdgen -l $l -d 120 -o /tmp; l=`expr $l + 1`; done

## Database schema

A DDL script is placed in the `./ddl/` directory.

## Test queries (decision support queries)

SQL scripts of the following test queries (4mQ.1 to 4mQ.6) are placed in the `./query/` directory.

- Production amount analysis (4mQ.1)
- Equipment availability analysis (4mQ.2)
- Production lead time analysis (4mQ.3)
- Quality test query (4mQ.4)
- Equipment failure influence analysis (4mQ.5)
- Production yield analysis (4mQ.6)

`[DATE]` must be substituted by a date value among the valid business dates. For example, if the dataset is generated for 12-day business operations, `[DATE]` must be any of 2022-04-01 to 2022-04-12. Below are some example date values for the datasets above noted.

| dataset | 4mQ2 | 4mQ3 | 4mQ4 | 4mQ5 | 4mQ6 |
| --- | --- | --- | --- | --- | --- | 
| Small scale | 2022-04-10 | 2022-04-01 | 2022-04-08 | 2022-04-03 | 2022-04-06 |
| Middle scale | 2022-04-15 | 2022-04-29 | 2022-04-22 | 2022-04-15 | 2022-04-15 |
| Large scale | 2022-04-25 | 2022-06-22 | 2022-06-20 | 2022-07-28 | 2022-07-03 |

## Original developers

- Kazuo Goda (UTokyo-IIS)
- Yuto Hayamizu (UTokyo-IIS)

## Contributors

- Shinji Fujiwara (Hitachi)
- Norifumi Nishikawa (Hitachi)

## License

4mbench is under MIT license. See LICENSE.
