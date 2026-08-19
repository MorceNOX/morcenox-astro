import sqlite3
from datetime import datetime
import pytz
from timezonefinder import TimezoneFinder

DB_NAME = "astro.db"
tf = TimezoneFinder()

conn = sqlite3.connect(DB_NAME)
cursor = conn.cursor()

# 1. Criação das colunas
for coluna in [("timezone", "TEXT"), ("gmt_offset_standard", "TEXT"), ("gmt_offset_secs", "INTEGER")]:
    try:
        cursor.execute(f"ALTER TABLE cities ADD COLUMN {coluna[0]} {coluna[1]};")
        conn.commit()
    except sqlite3.OperationalError:
        pass

cursor.execute("""
    SELECT rowid, lat, lon 
    FROM cities 
    WHERE timezone IS NULL 
       OR gmt_offset_standard IS NULL 
       OR gmt_offset_secs IS NULL
       """)
cidades = cursor.fetchall()

if not cidades:
    print("Todas as cidades já possuem timezone e offset padrão!")
    conn.close()
    exit()

dados_para_atualizar = []

for rowid, lat, lng in cidades:
    try:
        tz_iana = tf.timezone_at(lng=float(lng), lat=float(lat))
        if tz_iana:
            tz = pytz.timezone(tz_iana)
            
            # Buscamos o offset padrão (onde dst == 0) olhando o histórico do fuso
            # Procuramos um ponto no tempo que não esteja em DST (geralmente janeiro no hemisfério norte ou julho no sul)
            offset_padrao_segundos = None
            for utcoffset, dstoffset, tzname in tz._transition_info:
                if dstoffset == 0:  # Garante que o offset do Horário de Verão é ZERO
                    offset_padrao_segundos = utcoffset.total_seconds()
                    break
            
            # Fallback caso a iteração falhe (usa o fuso padrão histórico não-DST)
            if offset_padrao_segundos is None:
                offset_padrao_segundos = tz.utcoffset(datetime(2025, 1, 1)).total_seconds()

            # Formata em string (+HH:MM ou -HH:MM)
            horas = int(offset_padrao_segundos // 3600)
            minutos = int((abs(offset_padrao_segundos) % 3600) // 60)
            sinal = "+" if horas >= 0 else "-"
            gmt_offset_std = f"{sinal}{abs(horas):02d}:{minutos:02d}"
            
            dados_para_atualizar.append((tz_iana, gmt_offset_std, int(offset_padrao_segundos), rowid))
    except Exception:
        continue

# 2. Gravação em lote
if dados_para_atualizar:
    cursor.executemany(
        "UPDATE cities SET timezone = ?, gmt_offset_standard = ?, gmt_offset_secs = ? WHERE rowid = ?", 
        dados_para_atualizar
    )
    conn.commit()
    print(f"{len(dados_para_atualizar)} cidades configuradas com sucesso!")

conn.close()

