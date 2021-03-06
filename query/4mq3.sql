/* Production lead time analysis (4mQ.3) */

WITH RECURSIVE R(ML_LID, ML_MLID_MT09, WORK_NAME, ML_MLID, ML_MTYPE, ML_OLID_SRC, ML_OLID_DST, OL_TSBEGIN, OL_TSEND) AS (
  SELECT ML_LID, ML_MLID,
         CASE WHEN OL_PID IN (8, 7, 6) THEN 'W03' ELSE 
            CASE WHEN OL_PID IN (5, 4) THEN 'W02'ELSE CASE WHEN OL_PID IN (3, 2, 1, 0) THEN 'W01' END END END,
         ML_MLID, ML_MTYPE, ML_OLID_SRC, ML_OLID_DST, OL_TSBEGIN, OL_TSEND
  FROM MATERIAL_LOG, OPERATION_LOG, PROCEDURE
  WHERE ML_LID=OL_LID AND ML_OLID_SRC=OL_OLID AND OL_PID=P_PID
    AND OL_TSEND BETWEEN DATE'[DATE]' AND DATE'[DATE]' + INTERVAL '1 DAY'
    AND OL_LID = 109 AND P_PNAME='PROCEDURE000008 (STORAGING)'
  UNION ALL
  SELECT R.ML_LID, R.ML_MLID_MT09,
         CASE WHEN O.OL_PID IN (8, 7, 6) THEN 'W03' ELSE 
           CASE WHEN O.OL_PID IN (5, 4) THEN 'W02' ELSE CASE WHEN O.OL_PID IN (3, 2, 1, 0) THEN 'W01' END END END,
         M.ML_MLID, M.ML_MTYPE, M.ML_OLID_SRC, M.ML_OLID_DST, O.OL_TSBEGIN, O.OL_TSEND 
  FROM R, MATERIAL_LOG M, OPERATION_LOG O, PROCEDURE P
  WHERE R.ML_LID = M.ML_LID AND R.ML_OLID_SRC = M.ML_OLID_DST
    AND M.ML_MTYPE IN ('MT08', 'MT07', 'MT06', 'MT05', 'MT04', 'MT03', 'MT02', 'MT01', 'MT00')
    AND M.ML_LID=O.OL_LID AND M.ML_OLID_SRC=O.OL_OLID
    AND O.OL_PID=P.P_PID
)
SELECT ML_LID, HR, WORK_NAME, MAX(LEADTIME), MIN(LEADTIME), AVG(LEADTIME), SUM(CNT) 
FROM (
  SELECT ML_LID, ML_MLID_MT09, WORK_NAME, HR, W_TSEND - W_TSBEGIN AS LEADTIME, COUNT(*) AS CNT 
  FROM (
    SELECT ML_LID, ML_MLID_MT09, WORK_NAME, 
           MIN(OL_TSBEGIN) OVER (PARTITION BY ML_LID, ML_MLID_MT09, WORK_NAME) AS W_TSBEGIN, 
           MAX(OL_TSEND) OVER (PARTITION BY ML_LID, ML_MLID_MT09, WORK_NAME) AS W_TSEND, 
           DATE_TRUNC('HOUR', OL_TSEND) AS HR  
    FROM R
  ) A 
  GROUP BY  ML_LID, ML_MLID_MT09, HR, WORK_NAME, W_TSEND - W_TSBEGIN  
) B 
GROUP BY ML_LID, HR, WORK_NAME 
ORDER BY HR, WORK_NAME
;

