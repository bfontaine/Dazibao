if [ $# -ne 1 ]
then
    echo "usage: ./mk_large_dzb.sg <dzb_name>"
    exit 1
fi

touch max_size_tlv
truncate --size=16777215 max_size_tlv

../dazibao create $1

while [ $(stat --printf="%s" $1) -lt 3221225472 ] # 3GB
do
    ../dazibao add 2 $1 < max_size_tlv
done

rm max_size_tlv