/* Equipment failure influence analysis (4mQ.5a) */

-- [DATE] value: 2022-07-28 (default)
--               2022-04-28 (for 40 production days)

WITH T_OUT (LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, MLID_SRC, OL_TSEND, ML_MTYPE, ML_MLID, ML_OLID_SRC, ML_OLID_DST) AS (
  SELECT EL_LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, ML_MLID, OL_TSEND, ML_MTYPE, ML_MLID, ML_OLID_SRC, ML_OLID_DST 
  FROM EQUIPMENT INNER JOIN EQUIPMENT_LOG ON EL_EID=E_EID AND EL_LID=E_LID 
                 INNER JOIN OPERATION_LOG ON EL_EID=OL_EID AND EL_LID=OL_LID AND OL_TSBEGIN=EL_TS 
                 INNER JOIN MATERIAL_LOG  ON OL_LID=ML_LID AND OL_OLID=ML_OLID_SRC 
  WHERE EL_EID=4 AND EL_SENSOR='PRESSURE' AND (EL_READING < 10 OR EL_READING > 20)
    AND EL_TS BETWEEN DATE'[DATE]' AND DATE'[DATE]' + 1 DAY
  UNION ALL 
  SELECT T.LID, T.E_EID, T.E_ENAME, T.EL_SENSOR, T.EL_READING, T.MLID_SRC, T.OL_TSEND, M.ML_MTYPE, M.ML_MLID, M.ML_OLID_SRC, M.ML_OLID_DST 
  FROM T_OUT T INNER JOIN MATERIAL_LOG M ON T.LID=M.ML_LID AND T.ML_OLID_DST=M.ML_OLID_SRC
  WHERE M.ML_MTYPE IN ('MT09', 'MT08', 'MT07', 'MT06')
)
SELECT LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, MLID_SRC, OL_TSEND, ML_MTYPE, ML_MLID
FROM T_OUT 
;

/* Equipment failure influence analysis (4mQ.5b) */

-- [DATE] value: 2022-07-28 (default)
--               2022-04-28 (for 40 production days)

WITH T_IN (LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, MLID_SRC, OL_TSEND, ML_MTYPE, ML_MLID, ML_OLID_SRC, ML_OLID_DST) AS (
  SELECT EL_LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, ML_MLID, OL_TSEND, ML_MTYPE, ML_MLID, ML_OLID_SRC, ML_OLID_DST 
  FROM EQUIPMENT INNER JOIN EQUIPMENT_LOG ON EL_EID=E_EID AND EL_LID=E_LID 
                 INNER JOIN OPERATION_LOG ON EL_EID=OL_EID AND EL_LID=OL_LID AND OL_TSBEGIN=EL_TS 
                 INNER JOIN MATERIAL_LOG  ON OL_LID=ML_LID AND OL_OLID=ML_OLID_SRC 
  WHERE EL_EID=4 AND EL_SENSOR='PRESSURE' AND (EL_READING < 10 OR EL_READING > 20)
    AND EL_TS BETWEEN DATE'[DATE]' AND DATE'[DATE]' + 1 DAY
  UNION ALL 
  SELECT T.LID, T.E_EID, T.E_ENAME, T.EL_SENSOR, T.EL_READING, T.MLID_SRC, T.OL_TSEND, M.ML_MTYPE, M.ML_MLID, M.ML_OLID_SRC, M.ML_OLID_DST 
  FROM T_IN T INNER JOIN MATERIAL_LOG M ON T.LID=M.ML_LID AND T.ML_OLID_SRC=M.ML_OLID_DST
  WHERE M.ML_MTYPE IN ('MT04', 'MT03', 'MT02', 'MT01', 'MT00')
)
SELECT LID, E_EID, E_ENAME, EL_SENSOR, EL_READING, MLID_SRC, OL_TSEND, ML_MTYPE, ML_MLID
FROM T_IN 
;