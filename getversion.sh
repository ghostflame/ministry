OS=$(uname)

case $OS in
	Linux)
		sedOpt='r'
		;;
	Darwin)
		sedOpt='E'
		;;
esac

sed -${sedOpt} -n 's/^Version:[ 	](.*)/\1/p' ministry.spec
