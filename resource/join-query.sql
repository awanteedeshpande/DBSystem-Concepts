SELECT T0.id, T1.id, T2.id
FROM T0, T1, T2
WHERE T0.id = T1.fid_T0 AND T1.id = T2.fid_T1;
