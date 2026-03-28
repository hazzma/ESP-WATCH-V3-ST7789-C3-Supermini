---
name: RTC Memory
description: Penggunaan RTC_DATA_ATTR untuk menyimpan data antar deep sleep.
---

# RTC Memory

## Core Capabilities
- **Static Persistence**: Mendeklarasikan variabel dengan atribut `RTC_DATA_ATTR` agar data tetap tersimpan di memori khusus yang tidak mati saat deep sleep.
- **Data Integrity**: Meminimalkan penggunaan RTC memory (hanya untuk state kritis seperti AOD status, last HR reading, atau system boot count).
- **Wake Scene Recovery**: Membaca data RTC segera setelah bangun untuk menentukan transisi UI yang tepat (misal: kembali ke watchface dengan data HR terakhir yang masih valid).
