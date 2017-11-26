print_pa (struct mm_struct *mm, unsigned long va) 
{
    pgd t *pgd = pgd_offset(mm, va);
 
    if (pgd_present(*pgd)) 
    {
        pmd t *pmd = pmd_offset(pgd, va);
        if (pmd_present(*pmd)) 
        {
            pte_t *pte = pte_offset(pmd, va);
            if (pte_present(*pte))
             {
                printk("va 0x%lx -> pa 0x%lx\n",va, page_to_phys(pte_page(*pte));
             }
        }
    }
}