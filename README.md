# 4mbench

4mbench is a tool for performance benchmark of manufacturing business datababase.

## Dataset generator

A dataset generator *4mdgen* is placed in the `4mdgen/` directory.

### Build the dataset generator

	$ make

### Generate a dataset of 10-day business operations of the production line #101 in the `/tmp` directory

	$ 4mdgen -l 101 -d 10 -o /tmp

### Generate a dataset of 10-day business operations of the production lines #101 to #110 in the `/tmp` directory

	$ l=101; while [ $l -lt 111 ]; do 4mdgen -l $l -d 10 -o /tmp; l=`expr $l + 1`; done

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

## Original developers

- Kazuo Goda (UTokyo-IIS)
- Yuto Hayamizu (UTokyo-IIS)

## Contributors

- Shinji Fujiwara (Hitachi)
- Norifumi Nishikawa (Hitachi)

## License

4mbench is under MIT license. See LICENSE.
