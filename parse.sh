instanceName="TestInstance"
startDate="2022-01-25"
endDate="2022-01-25"
strategyName="ResistanceStrategy"
sharedObjectFileName="Resistance.so"
SYMBOLS="DIA"

strategyFolderPath="/vagrant/ie421/strategy"
sharedObjectOriginPath="$strategyFolderPath/$sharedObjectFileName"
sharedObjectDestinationPath="/home/vagrant/ss/bt/strategies_dlls"

# python3 /vagrant/iexdownloaderparser/src/download_iex_pcaps.py --start-date $startDate --end-date $endDate --download-dir /vagrant/iexdownloaderparser/data/iex_downloads


for pcap in $(ls /vagrant/iexdownloaderparser/data/iex_downloads/DEEP/*gz);
do
  pcap_date=$(echo $pcap | sed -r 's/.*data_feeds_(.*)_(.*)_IEXTP.*/\1/')
  echo "PCAP_FILE=$pcap PCAP_DATE=$pcap_date"
  cd /vagrant/iexdownloaderparser/ ; gunzip -d -c $pcap | tcpdump -r - -w - -s 0 | pypy3 /vagrant/iexdownloaderparser/src/parse_iex_pcap.py /dev/stdin --symbols $SYMBOLS --trade-date $pcap_date --output-deep-books-too

done;



