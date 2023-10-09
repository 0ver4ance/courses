def get_table_indxs(log_addr):
    '''Функция из логического адреса получает индексы таблиц PML4,
    Directory PTR, Directory, Table'''
    table_indxs = (
        log_addr >> 39 & 0x1FF,
        log_addr >> 30 & 0x1FF,
        log_addr >> 21 & 0x1FF,
        log_addr >> 12 & 0x1FF
    )
    return table_indxs

def get_phy_addr(log_addr, pml_4_addr, memory):
    '''Функция транслирует логический адрес в физический'''
    table_indxs = get_table_indxs(log_addr)
    curr_addr = pml_4_addr
    for indx in table_indxs:
        curr_addr += indx * 8
        if curr_addr not in memory:
            return "fault"
        if memory[curr_addr] % 2 == 0:
            return "fault"
        curr_addr = memory[curr_addr] & 0xFFFFFFFFFF000
    offset = log_addr & 0xFFF
    phy_addr = curr_addr + offset
    return phy_addr
        


if __name__ == "__main__":
    memory = dict()
    log_addrs = list()
    with open("input.txt") as file:
        cnt_mem_wr, cnt_log_addrs, pml_4_addr = map(int, file.readline().split())
        for _ in range(cnt_mem_wr):
            key, value = map(int, file.readline().split())
            memory[key] = value
        for _ in range(cnt_log_addrs):
            log_addrs.append(int(file.readline()))
    
    with open("output.txt", "w") as file:
        for log_addr in log_addrs:
            phy_addr = get_phy_addr(log_addr, pml_4_addr, memory)
            file.write(f"{phy_addr}\n")
