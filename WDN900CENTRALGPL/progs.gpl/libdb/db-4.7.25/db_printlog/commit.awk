# $Id: commit.awk,v 1.1.1.1 2009/10/09 03:02:13 jack Exp $
#
# Output tid of committed transactions.

/txn_regop/ {
	print $5
}
