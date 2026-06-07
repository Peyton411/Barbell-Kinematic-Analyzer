# -*- coding: utf-8 -*-
"""
Created on Mon Jun  1 17:55:07 2026

@author: Peyto
"""

import pandas as pd

# 1. Read the  CSV file
df = pd.read_csv("kinematic_data.csv")

# 2. Keep time and velocity
df_clean = df[["now_val", "vz_val"]]

# 3. Change the value
df_clean.to_csv("imu_data_prova.csv", index=False)
print("Good csv file is ready")