/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_xa_ext_h_
#define	_xa_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __xa_get_txn __P((ENV *, DB_TXN **, int));
int __db_xa_create __P((DB *));
int __db_rmid_to_env __P((int, ENV **));
int __db_xid_to_txn __P((ENV *, XID *, roff_t *));
int __db_map_rmid __P((int, ENV *));
int __db_unmap_rmid __P((int));
int __db_map_xid __P((ENV *, XID *, TXN_DETAIL *));
void __db_unmap_xid __P((ENV *, XID *, size_t));

#if defined(__cplusplus)
}
#endif
#endif /* !_xa_ext_h_ */
