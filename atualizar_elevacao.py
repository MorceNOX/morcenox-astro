import sqlite3
import srtm
import os

# --- CONFIGURAÇÃO ---
DB_NAME = "astro.db"         # Substitua pelo nome real do seu arquivo .db
TABLE_NAME = "cities"         # Nome da sua tabela de cidades
LAT_COLUMN = "lat"       # Nome da coluna de Latitude (REAL)
LON_COLUMN = "lon"      # Nome da coluna de Longitude (REAL)
ELEV_COLUMN = "elev"     # Nome da nova coluna de elevação
BATCH_SIZE = 6000             # No Linux, podemos usar blocos maiores para acelerar

def verificar_banco():
    """Verifica se o arquivo do banco de dados realmente existe no diretório."""
    if not os.path.exists(DB_NAME):
        print(f"❌ Erro: O arquivo '{DB_NAME}' não foi encontrado neste diretório.")
        print(f"Diretório atual: {os.getcwd()}")
        return False
    return True

def update_elevations():
    if not verificar_banco():
        return

    print("🚀 Inicializando o buscador de dados de elevação SRTM...")
    # O srtm.py criará automaticamente uma pasta oculta ~/.srtm/ para salvar o cache no seu Linux
    elevation_data = srtm.get_data()

    conn = sqlite3.connect(DB_NAME)
    cursor = conn.cursor()

    # Passo 1: Criar a coluna de elevação caso ela não exista
    try:
        cursor.execute(f"ALTER TABLE {TABLE_NAME} ADD COLUMN {ELEV_COLUMN} REAL;")
        conn.commit()
        print(f"ℹ️ Coluna '{ELEV_COLUMN}' criada com sucesso na tabela '{TABLE_NAME}'.")
    except sqlite3.OperationalError:
        print(f"ℹ️ A coluna '{ELEV_COLUMN}' já existe. Atualizando apenas registros vazios.")

    # Passo 2: Buscar apenas as linhas que ainda não possuem elevação
    cursor.execute(f"SELECT rowid, {LAT_COLUMN}, {LON_COLUMN} FROM {TABLE_NAME} WHERE {ELEV_COLUMN} IS NULL")
    rows = cursor.fetchall()
    total_rows = len(rows)
    print(f"📊 Total de cidades para atualizar: {total_rows}")

    if total_rows == 0:
        print("✅ Todas as cidades já possuem dados de elevação.")
        conn.close()
        return

    # Passo 3: Processamento e atualização em lote (Batch)
    updated_records = []
    counter = 0

    print("\n⏳ Processando... O primeiro lote pode demorar um pouco mais para baixar os arquivos do mapa local.\n")

    for rowid, lat, lon in rows:
        if lat is not None and lon is not None:
            try:
                # Busca a altitude em metros usando o arquivo SRTM local
                alt = elevation_data.get_elevation(lat, lon)
                
                # Caso a coordenada seja no oceano ou fora da cobertura, define como 0.0
                if alt is None:
                    alt = 0.0
                    
                updated_records.append((alt, rowid))
            except Exception as e:
                print(f"⚠️ Erro ao buscar dados para a linha ID {rowid}: {e}")
                continue

        # Gravação em lote no SQLite ao atingir o tamanho do BATCH
        if len(updated_records) >= BATCH_SIZE:
            cursor.executemany(f"UPDATE {TABLE_NAME} SET {ELEV_COLUMN} = ? WHERE rowid = ?", updated_records)
            conn.commit()
            counter += len(updated_records)
            print(f"➔ Progresso: {counter}/{total_rows} cidades atualizadas...")
            updated_records = []

    # Grava o lote final restante
    if updated_records:
        cursor.executemany(f"UPDATE {TABLE_NAME} SET {ELEV_COLUMN} = ? WHERE rowid = ?", updated_records)
        conn.commit()
        counter += len(updated_records)

    conn.close()
    print(f"\n🎉 Concluído! Sucesso ao atualizar {counter} cidades no banco SQLite.")

if __name__ == "__main__":
    update_elevations()

